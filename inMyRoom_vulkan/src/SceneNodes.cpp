#include "SceneNodes.h"

#include <cassert>

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"


SceneNodes::SceneNodes(const tinygltf::Model& in_model, const tinygltf::Scene& in_scene,
                       Anvil::BaseDevice* const in_device_ptr)
    : device_ptr(in_device_ptr)
{
    std::vector<glm::mat4> meshes_by_id_TRS;

    for (size_t i : in_scene.nodes)
    {
        const tinygltf::Node& this_node = in_model.nodes[i];
        const glm::mat4 node_global_trs_matrix = CreateTRSmatrix(this_node);

        Node root_node;
        root_node.globalTRSmatrix = node_global_trs_matrix;

        if (this_node.mesh > -1)
        {
            root_node.isMesh = true;
            root_node.meshID = static_cast<uint32_t>(meshes_by_id_TRS.size());
            root_node.meshIndex = this_node.mesh;

            nodes.emplace_back(root_node);
            meshes_by_id_TRS.emplace_back(node_global_trs_matrix);
        }
        else if (!this_node.children.empty())
        {
            root_node.isMesh = false;
            root_node.childrenCount = static_cast<uint32_t>(this_node.children.size());

            const size_t node_index = nodes.size();
            nodes.emplace_back(root_node);

            uint32_t node_and_children_size(0);
            for (int this_child : this_node.children)
            {
                node_and_children_size += AddNode(in_model, in_model.nodes[this_child], node_global_trs_matrix,
                                                  std::ref(meshes_by_id_TRS));
            }

            nodes[node_index].nodeAndChildrenSize = node_and_children_size;
        }
    }

    globalTRSmatrixesCount = meshes_by_id_TRS.size();
    globalTRSmatrixesBuffer_uptr = CreateBufferForTRSmatrixesAndCopy(meshes_by_id_TRS);

    {   // Create descriptor set group
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> new_dsg_create_info_ptr;
        new_dsg_create_info_ptr.resize(1);

        Anvil::DescriptorSetGroupUniquePtr new_dsg_ptr;

        new_dsg_create_info_ptr[0] = Anvil::DescriptorSetCreateInfo::create();

        new_dsg_create_info_ptr[0]->add_binding(0, /* in_binding */
                                                Anvil::DescriptorType::STORAGE_BUFFER,
                                                1, /* in_n_elements */
                                                Anvil::ShaderStageFlagBits::VERTEX_BIT);

        new_dsg_ptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                        { new_dsg_create_info_ptr },
                                                        false); /* in_releaseable_sets */

        new_dsg_ptr->set_binding_item(0, /* n_set         */
                                      0, /* binding_index */
                                      Anvil::DescriptorSet::UniformBufferBindingElement(globalTRSmatrixesBuffer_uptr.get()));

        TRSmatrixDescriptorSetGroup_uptr = std::move(new_dsg_ptr);
    }
}

SceneNodes::~SceneNodes()
{
    globalTRSmatrixesBuffer_uptr.reset();
}

void SceneNodes::BindNodesMeshes(NodesMeshes* in_nodesMeshes)
{
    nodesMeshes_ptr = in_nodesMeshes;
}

void SceneNodes::Draw(size_t in_primitivesSet_index, Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                      std::vector<Anvil::DescriptorSet*> in_low_descriptor_sets_ptrs)
{
    assert(nodesMeshes_ptr != nullptr);

    for (const Node& this_node : nodes)
        if (this_node.isMesh)
        {
            nodesMeshes_ptr->Draw(this_node.meshIndex, this_node.meshID, in_primitivesSet_index, in_cmd_buffer_ptr,
                                  in_low_descriptor_sets_ptrs);
        }
}

uint32_t SceneNodes::AddNode(const tinygltf::Model& in_model, const tinygltf::Node& in_node,
                             const glm::mat4 parentTRSmatrix, std::vector<glm::mat4>& meshesByIdTRS)
{
    glm::mat4 node_local_trs_matrix = CreateTRSmatrix(in_node);
    glm::mat4 parent_trs_matrix = parentTRSmatrix * node_local_trs_matrix;

    Node thisNode;
    thisNode.globalTRSmatrix = parent_trs_matrix;

    uint32_t nodeAndChildrenSize(1);

    if (in_node.mesh > -1)
    {
        thisNode.isMesh = true;
        thisNode.meshID = static_cast<uint32_t>(meshesByIdTRS.size());
        thisNode.meshIndex = in_node.mesh;

        nodes.emplace_back(thisNode);
        meshesByIdTRS.emplace_back(parent_trs_matrix);
    }
    else if (!in_node.children.empty())
    {
        thisNode.isMesh = false;
        thisNode.childrenCount = static_cast<uint32_t>(in_node.children.size());

        size_t nodeIndex = nodes.size();
        nodes.emplace_back(thisNode);

        for (size_t i = 0; i < in_node.children.size(); i++)
        {
            nodeAndChildrenSize += AddNode(in_model, in_model.nodes[in_node.children[i]], parent_trs_matrix,
                                           std::ref(meshesByIdTRS));
        }

        nodes[nodeIndex].nodeAndChildrenSize = nodeAndChildrenSize;
    }

    return nodeAndChildrenSize;
}

Anvil::BufferUniquePtr SceneNodes::CreateBufferForTRSmatrixesAndCopy(const std::vector<glm::mat4>& meshesByIdTRS) const
{
    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    meshesByIdTRS.size() * sizeof(glm::mat4),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    Anvil::BufferUsageFlagBits::STORAGE_BUFFER_BIT);

    Anvil::BufferUniquePtr buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    allocator_ptr->add_buffer(buffer_ptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE);

    buffer_ptr->write(0,
                      meshesByIdTRS.size() * sizeof(glm::mat4),
                      meshesByIdTRS.data());

    return std::move(buffer_ptr);
}

glm::mat4 SceneNodes::CreateTRSmatrix(const tinygltf::Node& in_node) const
{
    if (in_node.matrix.size() == 16)
    {
        return  glm::mat4( 1.f, 0.f, 0.f, 0.f,
                           0.f,-1.f, 0.f, 0.f,
                           0.f, 0.f,-1.f, 0.f,
                           0.f, 0.f, 0.f, 1.f) *
                glm::mat4(in_node.matrix[0] , in_node.matrix[1] , in_node.matrix[2] , in_node.matrix[3] ,
                          in_node.matrix[4] , in_node.matrix[5] , in_node.matrix[6] , in_node.matrix[7] ,
                          in_node.matrix[8] , in_node.matrix[9] , in_node.matrix[10], in_node.matrix[11],
                          in_node.matrix[12], in_node.matrix[13], in_node.matrix[14], in_node.matrix[15]) *
                glm::mat4(1.f, 0.f, 0.f, 0.f,
                          0.f,-1.f, 0.f, 0.f,
                          0.f, 0.f,-1.f, 0.f,
                          0.f, 0.f, 0.f, 1.f);
    }
    else
    {
        glm::mat4 return_matrix = glm::mat4(1.0f);
        if (in_node.scale.size() == 3)
        {
            return_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(in_node.scale[0], in_node.scale[1], in_node.scale[2])) * return_matrix;
        }
        if (in_node.rotation.size() == 4)
        {
            const glm::qua<float> rotation_qua(static_cast<float>(in_node.rotation[3]), static_cast<float>(in_node.rotation[0]), static_cast<float>(-in_node.rotation[1]), static_cast<float>(-in_node.rotation[2]));

            return_matrix = glm::toMat4<float, glm::packed_highp>(rotation_qua) * return_matrix;
        }
        if (in_node.translation.size() == 3)
        {
            return_matrix = translate(glm::mat4(1.0f), glm::vec3( in_node.translation[0], -in_node.translation[1], -in_node.translation[2])) * return_matrix;
        }

        return return_matrix;
    }
}
