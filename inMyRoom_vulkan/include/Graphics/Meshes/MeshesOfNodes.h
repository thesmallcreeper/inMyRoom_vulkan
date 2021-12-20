#pragma once

#include <vector>

#include "tiny_gltf.h"

#include "Geometry/OBBtree.h"
#include "Graphics/Meshes/PrimitivesOfMeshes.h"

struct MeshInfo
{
    bool isSkinned = false;
    std::vector<float> morphDefaultWeights = {};

    std::vector<size_t> primitivesIndex;
    OBBtree boundBoxTree;

    bool IsSkinned() const {return isSkinned;}
    bool HasMorphTargets() const {return morphDefaultWeights.size();}
};


class MeshesOfNodes
{
public: // functions
    explicit MeshesOfNodes(PrimitivesOfMeshes* in_primitivesOfMeshes_ptr);

    void AddMeshesOfModel(const tinygltf::Model& in_model);

    size_t GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const;
    const MeshInfo& GetMeshInfo(size_t this_mesh_index) const {return meshes[this_mesh_index];};

private: // data
    std::vector<MeshInfo> meshes;
    std::unordered_map<tinygltf::Model*, size_t> modelToMeshIndexOffset_umap;

    PrimitivesOfMeshes* primitivesOfMeshes_ptr;
};
