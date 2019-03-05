#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/quaternion.hpp"

#include "tiny_gltf.h"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "SceneMeshes.h"

struct Node
{
	bool isMesh;
	union {
		struct {
			uint32_t nodeAndChildrenSize;
			uint32_t childrenCount;
		};	
		struct {
			uint32_t meshIndex;
			uint32_t meshID;
		};		
	};
	glm::mat4 globalTRSmatrix;
};

class NodesTree
{
public:
	NodesTree(const tinygltf::Model& in_model, const tinygltf::Scene &in_scene, Anvil::BaseDevice* in_device_ptr);
	~NodesTree();

	void Draw(SceneMeshes& in_scenemeshes, Anvil::PrimaryCommandBuffer* in_command_buffer, Anvil::DescriptorSet* in_dsg_ptr, Anvil::BaseDevice* in_device_ptr);

	Anvil::BufferUniquePtr globalTRSmatrixesBuffer;
	size_t globalTRSmatrixesCount;

private:
	uint32_t AddNode(const tinygltf::Model& in_model, const tinygltf::Node& in_node, const glm::mat4 parentTRSmatrix, std::vector<glm::mat4>& meshesByIdTRS);
	Anvil::BufferUniquePtr CreateBufferForTRSmatrixesAndCopy(const std::vector<glm::mat4>& meshesByIdTRS, Anvil::BaseDevice* in_device_ptr);
	glm::mat4 CreateTRSmatrix(const tinygltf::Node& in_node);

	std::vector<Node> nodes;

};