#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/mat4x4.hpp"

#include "tiny_gltf.h"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "Math/float4x4.h"
#include "Geometry/AABB.h"
#include "Geometry/PBVolume.h"

#include "MeshesOfNodes.h"
#include "Drawer.h"

struct NodeRef
{
    NodeRef()
        :isMesh(false)
    {}

    uint32_t nodeAndChildrenSize;
    uint32_t childrenCount;
    math::AABB globalAABB;

    uint32_t meshIndex;
    uint32_t objectID;
    math::float4x4 globalTRSmatrix;

    bool isMesh;
};

struct NodeRecursive
{
    uint32_t childrenSoFar;
    uint32_t childrenCount;
};

class NodesOfScene
{
public:
    NodesOfScene(const tinygltf::Model& in_model, const tinygltf::Scene& in_scene,
                 MeshesOfNodes* in_meshesOfNodes_ptr, Anvil::BaseDevice* const in_device_ptr);
    ~NodesOfScene();

    std::vector<DrawRequest> DrawUsingFrustumCull(const std::array<math::Plane, 6> in_viewport_planes);

public:
	std::vector<NodeRef> nodes;

    Anvil::DescriptorSetGroupUniquePtr TRSmatrixDescriptorSetGroup_uptr;
    Anvil::BufferUniquePtr globalTRSmatrixesBuffer_uptr;
    size_t globalTRSmatrixesCount;

private:
    math::AABB AddNode(const tinygltf::Model& in_model, const tinygltf::Node& in_node, glm::mat4 parentTRSmatrix,
                       std::vector<glm::mat4>& meshesByIdTRS);

    Anvil::BufferUniquePtr CreateBufferForTRSmatrixesAndCopy(const std::vector<glm::mat4>& meshesByIdTRS) const;

    glm::mat4 CreateTRSmatrix(const tinygltf::Node& in_node) const;

private:
    Anvil::BaseDevice* const device_ptr;

    MeshesOfNodes* meshesOfNodes_ptr;
};
