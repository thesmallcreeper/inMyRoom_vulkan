#include "Graphics/Meshes/MeshesOfNodes.h"


MeshesOfNodes::MeshesOfNodes(PrimitivesOfMeshes* in_primitivesOfMeshes_ptr)
    :
    primitivesOfMeshes_ptr(in_primitivesOfMeshes_ptr)
{
}

void MeshesOfNodes::AddMeshesOfModel(const tinygltf::Model& in_model)
{
    modelToMeshIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), meshes.size());

    for (const tinygltf::Mesh& this_mesh : in_model.meshes)
    {
        MeshInfo this_mesh_range;

        primitivesOfMeshes_ptr->StartRecordOBBtree();

        for (const tinygltf::Primitive& this_primitive : this_mesh.primitives)
            this_mesh_range.primitivesIndex.emplace_back(primitivesOfMeshes_ptr->AddPrimitive(in_model, this_primitive));

        this_mesh_range.boundBoxTree = primitivesOfMeshes_ptr->GetOBBtreeAndReset();

        meshes.emplace_back(std::move(this_mesh_range));
    }
}

size_t MeshesOfNodes::GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToMeshIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToMeshIndexOffset_umap.end());

    return search->second;
}

const MeshInfo* MeshesOfNodes::GetMeshInfoPtr(size_t this_mesh_index) const
{
    return &meshes[this_mesh_index];
}