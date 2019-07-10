#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/mat4x4.hpp"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "Math/float4x4.h"

#include "tiny_gltf.h"

#include "NodesMeshes.h"
#include "Drawer.h"

struct Node
{
    bool isMesh;

    union
    {
        struct
        {
            uint32_t nodeAndChildrenSize;
            uint32_t childrenCount;
        };

        struct
        {
            uint32_t meshIndex;
            uint32_t meshID;
        };
    };

    glm::mat4 globalTRSmatrix;
};

class SceneNodes
{
public:
    SceneNodes(const tinygltf::Model& in_model, const tinygltf::Scene& in_scene, Anvil::BaseDevice* const in_device_ptr);
    ~SceneNodes();

    void BindNodesMeshes(NodesMeshes* in_nodesMeshes);
    std::vector<DrawRequest> Draw(const std::array<math::Plane, 6> in_viewport_planes);

public:
	std::vector<Node> nodes;

    std::vector<math::float4x4> globalTRSperIDMatrixes;

    Anvil::DescriptorSetGroupUniquePtr TRSmatrixDescriptorSetGroup_uptr;
    Anvil::BufferUniquePtr globalTRSmatrixesBuffer_uptr;
    size_t globalTRSmatrixesCount;

private:
    uint32_t AddNode(const tinygltf::Model& in_model, const tinygltf::Node& in_node, glm::mat4 parentTRSmatrix,
                     std::vector<glm::mat4>& meshesByIdTRS);

    Anvil::BufferUniquePtr CreateBufferForTRSmatrixesAndCopy(const std::vector<glm::mat4>& meshesByIdTRS) const;

    glm::mat4 CreateTRSmatrix(const tinygltf::Node& in_node) const;

private:
    Anvil::BaseDevice* const device_ptr;

    NodesMeshes* nodesMeshes_ptr;

};
