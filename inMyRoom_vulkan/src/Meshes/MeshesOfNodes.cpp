#include "Meshes/MeshesOfNodes.h"


MeshesOfNodes::MeshesOfNodes(PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
                             Anvil::BaseDevice* const in_device_ptr)
    :
    primitivesOfMeshes_ptr(in_primitivesOfMeshes_ptr),
    device_ptr(in_device_ptr)
{
}

void MeshesOfNodes::AddMeshesOfModel(const tinygltf::Model& in_model)
{
    for (const tinygltf::Mesh& this_mesh : in_model.meshes)
    {
        MeshInfo this_mesh_range;

        primitivesOfMeshes_ptr->StartRecordOBB();

        this_mesh_range.primitiveFirstOffset = primitivesOfMeshes_ptr->GetPrimitivesCount();
        this_mesh_range.primitiveRangeSize = this_mesh.primitives.size();

        for (tinygltf::Primitive this_primitive : this_mesh.primitives)
            primitivesOfMeshes_ptr->AddPrimitive(in_model, this_primitive);

        this_mesh_range.boundBox = primitivesOfMeshes_ptr->GetOBBandReset();

        meshes.emplace_back(this_mesh_range);
    }

    modelToMeshIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), meshesSoFar);

    meshesSoFar += in_model.meshes.size();
}

size_t MeshesOfNodes::GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToMeshIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToMeshIndexOffset_umap.end());

    return search->second;
}

MeshInfo MeshesOfNodes::GetMesh(size_t this_mesh_index) const
{
    return meshes[this_mesh_index];
}