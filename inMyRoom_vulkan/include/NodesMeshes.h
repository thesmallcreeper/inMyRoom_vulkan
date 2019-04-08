#pragma once
#include <vector>

#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/command_buffer.h"

#include "tiny_gltf.h"

#include "MeshesPrimitives.h"

struct MeshRange
{
    size_t primitiveFirstOffset;
    size_t primitiveRangeSize;
};


class NodesMeshes
{
public:
    NodesMeshes(tinygltf::Model& in_model, MeshesPrimitives* in_meshesPrimitives_ptr, Anvil::BaseDevice* in_device_ptr);
    ~NodesMeshes();

    void Draw(size_t in_mesh, uint32_t in_meshDeviceID, size_t in_primitivesSet_index,
              Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
              std::vector<Anvil::DescriptorSet*> in_low_descriptor_sets_ptrs);

private:
    std::vector<MeshRange> meshes;

    MeshesPrimitives* meshesPrimitives_ptr;

    Anvil::BaseDevice* device_ptr;
};
