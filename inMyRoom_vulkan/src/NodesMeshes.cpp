#include "NodesMeshes.h"

#include "MeshesPrimitives.h"

NodesMeshes::NodesMeshes(tinygltf::Model& in_model, MeshesPrimitives* in_meshesPrimitives_ptr,
                         Anvil::BaseDevice* const in_device_ptr)
    :
    meshesPrimitives_ptr(in_meshesPrimitives_ptr),
    device_ptr(in_device_ptr)
{
    size_t primitives_so_far = 0;
    for (tinygltf::Mesh& this_mesh : in_model.meshes)
    {
        MeshRange this_mesh_range;
        this_mesh_range.primitiveFirstOffset = primitives_so_far;
        this_mesh_range.primitiveRangeSize = this_mesh.primitives.size();
        for (tinygltf::Primitive this_primitive : this_mesh.primitives)
            meshesPrimitives_ptr->AddPrimitive(in_model, this_primitive);

        meshes.emplace_back(this_mesh_range);
        primitives_so_far += this_mesh.primitives.size();
    }

    meshesPrimitives_ptr->FlashBuffersToDevice();
}

NodesMeshes::~NodesMeshes()
{
}

