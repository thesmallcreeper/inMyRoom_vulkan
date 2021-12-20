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
        MeshInfo this_mesh_info;
        size_t morphTargetsCount = 0;

        primitivesOfMeshes_ptr->StartRecordOBBtree();

        for (const tinygltf::Primitive& this_primitive : this_mesh.primitives) {
            size_t index = primitivesOfMeshes_ptr->AddPrimitive(in_model, this_primitive);

            this_mesh_info.primitivesIndex.emplace_back(index);
            this_mesh_info.isSkinned |= primitivesOfMeshes_ptr->IsPrimitiveSkinned(index);
            morphTargetsCount = std::max(primitivesOfMeshes_ptr->PrimitiveMorphTargetsCount(index), morphTargetsCount);
        }

        this_mesh_info.boundBoxTree = primitivesOfMeshes_ptr->GetOBBtreeAndReset();

        this_mesh_info.morphDefaultWeights = std::vector<float>(morphTargetsCount, 0.f);
        if (this_mesh.weights.size()) {
            assert(this_mesh_info.morphDefaultWeights.size() == this_mesh.weights.size());
            std::transform(this_mesh.weights.begin(), this_mesh.weights.end(), this_mesh_info.morphDefaultWeights.begin(),
                           [](double w) -> float { return float(w); });
        }

        meshes.emplace_back(std::move(this_mesh_info));
    }
}

size_t MeshesOfNodes::GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToMeshIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToMeshIndexOffset_umap.end());

    return search->second;
}
