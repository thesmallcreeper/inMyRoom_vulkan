#pragma once
#define GLM_ENABLE_EXPERIMENTAL

#include "glm/mat4x4.hpp"
#include "glm/gtx/transform.hpp"
#include "glm/gtx/quaternion.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "tiny_gltf.h"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "FrustumCulling.h"
#include "Plane.h"
#include "OBB.h"
#include "MeshesOfNodes.h"
#include "Drawer.h"

struct NodeInfo
{
    NodeInfo()
        :isMesh(false),
         shouldRender(true)
    {}
    bool isMesh;
    bool shouldRender;

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
            uint32_t objectID;
        };
    };

    glm::mat4x4 localTRSmatrix;
};

struct NodeRecursive
{
    uint32_t childrenSoFar;
    uint32_t childrenCount;
    glm::mat4x4 globalTRSmaxtix;
};

class NodesOfScene
{
public: // functions
    NodesOfScene(const tinygltf::Model& in_model, const tinygltf::Scene& in_scene,
                 MeshesOfNodes* in_meshesOfNodes_ptr, Anvil::BaseDevice* const in_device_ptr);

    std::vector<DrawRequest> DrawUsingFrustumCull(const std::array<Plane, 6> in_viewport_planes);

private: // data
    std::vector<NodeInfo> nodesInfos;

    uint32_t nextObjectsID = 0;

    void AddNode(const tinygltf::Model& in_model,
                 const tinygltf::Node& in_node);

    glm::mat4 CreateTRSmatrix(const tinygltf::Node& in_node) const;

    Anvil::BaseDevice* const device_ptr;

    MeshesOfNodes* meshesOfNodes_ptr;
};