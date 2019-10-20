#include "MeshesOfNodes.h"

#include "PrimitivesOfMeshes.h"

MeshesOfNodes::MeshesOfNodes(tinygltf::Model& in_model, PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
                             Anvil::BaseDevice* const in_device_ptr)
    :
    primitivesOfMeshes_ptr(in_primitivesOfMeshes_ptr),
    device_ptr(in_device_ptr)
{
    size_t primitives_so_far = 0;
    for (tinygltf::Mesh& this_mesh : in_model.meshes)
    {
        MeshRange this_mesh_range;

        primitivesOfMeshes_ptr->startRecordOBB();

        this_mesh_range.primitiveFirstOffset = primitivesOfMeshes_ptr->primitivesCount();
        this_mesh_range.primitiveRangeSize = this_mesh.primitives.size();

        for (tinygltf::Primitive this_primitive : this_mesh.primitives)
            primitivesOfMeshes_ptr->addPrimitive(in_model, this_primitive);

        this_mesh_range.boundBox = primitivesOfMeshes_ptr->getOBBandReset();

        meshes.emplace_back(this_mesh_range);

        primitives_so_far += this_mesh.primitives.size();
    }

    primitivesOfMeshes_ptr->flashBuffersToDevice();
}

MeshesOfNodes::~MeshesOfNodes()
{
}

