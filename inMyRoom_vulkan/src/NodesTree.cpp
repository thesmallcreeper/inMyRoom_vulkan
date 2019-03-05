#include "NodesTree.h"

NodesTree::NodesTree(const tinygltf::Model& in_model, const tinygltf::Scene &in_scene, Anvil::BaseDevice* in_device_ptr)
{
	std::vector<glm::mat4> meshesByIdTRS;

	for(size_t i: in_scene.nodes)
	{
		const tinygltf::Node& thisNode = in_model.nodes[i];
		glm::mat4 nodeGlobalTRSmatrix = CreateTRSmatrix(thisNode);

		Node rootNode;
		rootNode.globalTRSmatrix = nodeGlobalTRSmatrix;

		if (thisNode.mesh > -1)
		{
			rootNode.isMesh = true;
			rootNode.meshID = static_cast<uint32_t>(meshesByIdTRS.size());
			rootNode.meshIndex = thisNode.mesh;

			nodes.emplace_back(rootNode);
			meshesByIdTRS.emplace_back(nodeGlobalTRSmatrix * glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
																	   0.0f, -1.0f, 0.0f, 0.0f,
																	   0.0f, 0.0f, -1.0f, 0.0f,
																	   0.0f, 0.0f, 0.0f, 1.0f));
		}
		else if (thisNode.children.size())
		{
			rootNode.isMesh = false;
			rootNode.childrenCount = static_cast<uint32_t>(thisNode.children.size());

			size_t nodeIndex = nodes.size();
			nodes.emplace_back(rootNode);

			uint32_t nodeAndChildrenSize(0);
			for (size_t i = 0; i < thisNode.children.size(); i++)
			{
				nodeAndChildrenSize += AddNode(in_model, in_model.nodes[thisNode.children[i]], nodeGlobalTRSmatrix, std::ref(meshesByIdTRS));
			}

			nodes[nodeIndex].nodeAndChildrenSize = nodeAndChildrenSize;
		}
	}

	globalTRSmatrixesCount = meshesByIdTRS.size();
	globalTRSmatrixesBuffer = CreateBufferForTRSmatrixesAndCopy(meshesByIdTRS, in_device_ptr);
}

NodesTree::~NodesTree()
{
	globalTRSmatrixesBuffer.reset();
}

void NodesTree::Draw(SceneMeshes& in_scenemeshes, Anvil::PrimaryCommandBuffer* in_command_buffer, Anvil::DescriptorSet* in_dsg_ptr, Anvil::BaseDevice* in_device_ptr)
{
	for (const Node& thisNode : nodes)
		if (thisNode.isMesh == true)
		{
			in_scenemeshes.Draw(thisNode.meshIndex, thisNode.meshID, in_command_buffer, in_dsg_ptr, in_device_ptr);
		}
}

uint32_t NodesTree::AddNode(const tinygltf::Model& in_model, const tinygltf::Node& in_node, const glm::mat4 parentTRSmatrix, std::vector<glm::mat4>& meshesByIdTRS)
{
	glm::mat4 nodeLocalTRSmatrix = CreateTRSmatrix(in_node);
	glm::mat4 nodeGlobalTRSmartix = parentTRSmatrix * nodeLocalTRSmatrix;

	Node thisNode;
	thisNode.globalTRSmatrix = nodeGlobalTRSmartix;

	uint32_t nodeAndChildrenSize(1);

	if (in_node.mesh > -1)
	{
		thisNode.isMesh = true;
		thisNode.meshID = static_cast<uint32_t>(meshesByIdTRS.size());
		thisNode.meshIndex = in_node.mesh;

		nodes.emplace_back(thisNode);
		meshesByIdTRS.emplace_back(nodeGlobalTRSmartix * glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
																   0.0f, -1.0f, 0.0f, 0.0f,
																   0.0f, 0.0f, -1.0f, 0.0f,
																   0.0f, 0.0f, 0.0f, 1.0f));
	}
	else if (in_node.children.size())
	{
		thisNode.isMesh = false;
		thisNode.childrenCount = static_cast<uint32_t>(in_node.children.size());

		size_t nodeIndex = nodes.size();
		nodes.emplace_back(thisNode);

		for (size_t i = 0; i < in_node.children.size(); i++)
		{
			nodeAndChildrenSize += AddNode(in_model, in_model.nodes[in_node.children[i]], nodeGlobalTRSmartix, std::ref(meshesByIdTRS));
		}

		nodes[nodeIndex].nodeAndChildrenSize = nodeAndChildrenSize;
	}

	return nodeAndChildrenSize;
}

Anvil::BufferUniquePtr NodesTree::CreateBufferForTRSmatrixesAndCopy(const std::vector<glm::mat4>& meshesByIdTRS, Anvil::BaseDevice* in_device_ptr)
{
	auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(in_device_ptr,
																	meshesByIdTRS.size()*sizeof(glm::mat4),
																	Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
																	Anvil::SharingMode::EXCLUSIVE,
																	Anvil::BufferCreateFlagBits::NONE,
																	Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

	Anvil::BufferUniquePtr buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

	auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(in_device_ptr);

	allocator_ptr->add_buffer(buffer_ptr.get(),
							  Anvil::MemoryFeatureFlagBits::NONE);

	buffer_ptr->write(0,
					  meshesByIdTRS.size() * sizeof(glm::mat4),
					  meshesByIdTRS.data());

	return std::move(buffer_ptr);
}

glm::mat4 NodesTree::CreateTRSmatrix(const tinygltf::Node& in_node)
{
	if (in_node.matrix.size() == 16)
	{
		return glm::mat4(in_node.matrix[0] , in_node.matrix[1] , in_node.matrix[2] , in_node.matrix[3],
						 in_node.matrix[4] , in_node.matrix[5] , in_node.matrix[6] , in_node.matrix[7], 
						 in_node.matrix[8] , in_node.matrix[9] , in_node.matrix[10], in_node.matrix[11],
						 in_node.matrix[12], in_node.matrix[13], in_node.matrix[14], in_node.matrix[15]);
	}
	else
	{
		glm::mat4 returnMatrix = glm::mat4(1.0f);
		if (in_node.scale.size() == 3)
		{
			returnMatrix = glm::scale(glm::mat4(1.0f), glm::vec3(in_node.scale[0], in_node.scale[1], in_node.scale[2])) * returnMatrix;
		}
		if (in_node.rotation.size() == 4)
		{
			glm::qua rotationQua(in_node.rotation[3], in_node.rotation[0], -in_node.rotation[1], -in_node.rotation[2]);
			returnMatrix = glm::toMat4<float,glm::packed_highp>(rotationQua) * returnMatrix;
		}
		if (in_node.translation.size() == 3)
		{
			returnMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(in_node.translation[0], -in_node.translation[1], -in_node.translation[2])) * returnMatrix;
		}

		return returnMatrix;
	}
}