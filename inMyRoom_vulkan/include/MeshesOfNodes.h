#pragma once
#include <vector>

#include "tiny_gltf.h"

#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/command_buffer.h"

#include "OBB.h"

#include "PrimitivesOfMeshes.h"

struct MeshRange
{
    size_t primitiveFirstOffset;
    size_t primitiveRangeSize;
    OBB boundBox;
};


class MeshesOfNodes
{
public:
    MeshesOfNodes(tinygltf::Model& in_model, PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
                Anvil::BaseDevice* const in_device_ptr);
    ~MeshesOfNodes();

    std::vector<MeshRange> meshes;

private:
    Anvil::BaseDevice* const device_ptr;

    PrimitivesOfMeshes* primitivesOfMeshes_ptr;
};
