#pragma once
#include <vector>

#include "tiny_gltf.h"

#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/command_buffer.h"

#include "Geometry/Sphere.h"

#include "MeshesPrimitives.h"

struct MeshRange
{
    size_t primitiveFirstOffset;
    size_t primitiveRangeSize;
    math::Sphere boundSphere;
};


class NodesMeshes
{
public:
    NodesMeshes(tinygltf::Model& in_model, MeshesPrimitives* in_meshesPrimitives_ptr,
                Anvil::BaseDevice* const in_device_ptr);
    ~NodesMeshes();

    std::vector<MeshRange> meshes;

private:
    Anvil::BaseDevice* const device_ptr;

    MeshesPrimitives* meshesPrimitives_ptr;
};
