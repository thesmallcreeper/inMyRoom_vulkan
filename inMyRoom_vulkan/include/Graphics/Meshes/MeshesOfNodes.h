#pragma once

#include <vector>

#include "tiny_gltf.h"

#include "Geometry/OBBtree.h"
#include "Graphics/Meshes/PrimitivesOfMeshes.h"

struct MeshInfo
{
    struct MeshBLAS
    {
        bool hasBLAS = false;
        bool disableFaceCulling = false;
        vk::AccelerationStructureKHR handle;
        uint64_t deviceAddress = 0;

        vk::Buffer buffer;
        vma::Allocation allocation;
        size_t bufferSize;

        size_t buildScratchBufferSize;
        size_t updateScratchBufferSize;
    };

    std::vector<size_t> primitivesIndex;
    MeshBLAS meshBLAS;

    OBBtree boundBoxTree;

    bool isSkinned = false;
    std::vector<float> morphDefaultWeights = {};

    bool IsSkinned() const {return isSkinned;}
    bool HasMorphTargets() const {return morphDefaultWeights.size();}
};


class MeshesOfNodes
{
public: // functions
    MeshesOfNodes(PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
                  vk::Device device,
                  vma::Allocator vma_allocator);
    ~MeshesOfNodes();

    void AddMeshesOfModel(const tinygltf::Model& in_model);

    size_t GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const;
    const MeshInfo& GetMeshInfo(size_t this_mesh_index) const {assert(hasBeenFlashed); return meshes[this_mesh_index];};

    void FlashDevice(std::vector<std::pair<vk::Queue, uint32_t>> queues);

private: // data
    std::vector<MeshInfo> meshes;
    std::unordered_map<tinygltf::Model*, size_t> modelToMeshIndexOffset_umap;

    vk::Device device;
    vma::Allocator vma_allocator;

    bool hasBeenFlashed = false;

    PrimitivesOfMeshes* primitivesOfMeshes_ptr;
};
