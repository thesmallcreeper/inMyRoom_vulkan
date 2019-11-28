#pragma once
#include <vector>

#include "tiny_gltf.h"

#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/command_buffer.h"

#include "Geometry/OBB.h"

#include "Meshes/PrimitivesOfMeshes.h"

struct MeshInfo
{
    size_t primitiveFirstOffset;
    size_t primitiveRangeSize;
    OBB boundBox;
};


class MeshesOfNodes
{
public: // functions
    MeshesOfNodes(PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
                  Anvil::BaseDevice* const in_device_ptr);

    void AddMeshesOfModel(const tinygltf::Model& in_model);

    size_t GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const;

    MeshInfo GetMesh(size_t this_mesh_index) const;

private: // data
    size_t meshesSoFar;

    std::vector<MeshInfo> meshes;

    std::unordered_map<tinygltf::Model*, size_t> modelToMeshIndexOffset_umap;

    Anvil::BaseDevice* const device_ptr;

    PrimitivesOfMeshes* primitivesOfMeshes_ptr;
};
