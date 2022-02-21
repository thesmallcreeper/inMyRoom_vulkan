#pragma once

#include "Graphics/Meshes/PrimitivesOfMeshes.h"
#include "Graphics/PipelinesFactory.h"
#include "Graphics/ShadersSetsFamiliesCache.h"
#include "ECS/ECStypes.h"

#define MAX_MORPH_WEIGHTS 8

#include "vulkan/vulkan.hpp"
#include "vulkan/vulkan_hash.hpp"
#include "vk_mem_alloc.hpp"

struct DynamicMeshInfo {
    struct DynamicPrimitiveInfo {
        size_t primitiveIndex           = -1;

        VkDeviceSize positionByteOffset = -1;

        VkDeviceSize normalByteOffset   = -1;

        VkDeviceSize tangentByteOffset  = -1;

        int texcoordsCount              =  0;
        VkDeviceSize texcoordsByteOffset= -1;

        VkDeviceSize colorByteOffset    = -1;
    };

    std::vector<DynamicPrimitiveInfo> dynamicPrimitives;

    vk::Buffer buffer;
    vma::Allocation allocation;
    size_t halfSize                 =  0;
    size_t descriptorIndex          = -1;

    bool hasDynamicBLAS             = false;

    vk::AccelerationStructureKHR BLASesHandles[2];
    uint64_t BLASesDeviceAddresses[2] = {0, 0};
    vk::Buffer BLASesBuffer;
    vma::Allocation BLASesAllocation;
    size_t BLASesHalfSize           = 0;

    vk::Buffer updateScratchBuffer;
    vma::Allocation updateScratchAllocation;

    size_t meshIndex                = -1;

    bool shouldBeDeleted            = false;
};

struct DynamicMeshComputePushConstants {
    uint32_t matrixOffset = -1;
    uint32_t inverseMatricesOffset = -1;
    uint32_t verticesOffset = -1;
    uint32_t jointsOffset = -1;
    uint32_t weightsOffset = -1;
    uint32_t jointsGroupsCount = 0;
    uint32_t morphTargets = 0;
    uint32_t size_x = 0;
    uint32_t step_multiplier = 1;
    uint32_t resultDescriptorIndex = 0;
    uint32_t resultOffset = -1;
    std::array<float, MAX_MORPH_WEIGHTS> morph_weights = {};
};

class DynamicMeshes
{
public:
    DynamicMeshes(class Graphics* in_graphics_ptr,
                  vk::Device device, vma::Allocator vma_allocator,
                  size_t max_dynamicMeshes);
    ~DynamicMeshes();

    void FlashDevice();

    const DynamicMeshInfo& GetDynamicMeshInfo(size_t index) const;
    size_t AddDynamicMesh(size_t mesh_index);
    void RemoveDynamicMeshSafe(size_t index);

    void RecordTransformations(vk::CommandBuffer command_buffer,
                               const std::vector<DrawInfo>& draw_infos);

    vk::DescriptorSet GetDescriptorSet() {return descriptorSets[swapIndex % 3];}
    vk::DescriptorSetLayout GetDescriptorLayout() {return descriptorSetLayout;}
    void SwapDescriptorSet(size_t swap_index);
    void CompleteRemovesSafe();

private:
    std::unordered_map<size_t, DynamicMeshInfo> indexToDynamicMeshInfo_umap;
    std::vector<std::pair<size_t, uint32_t>> indexToBeRemovedCountdown;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       descriptorSets[3];
    vk::DescriptorSetLayout descriptorSetLayout;
    const size_t max_dynamicMeshes;

    vk::PipelineLayout  computeLayout;
    vk::Pipeline        positionCompPipeline;
    vk::Pipeline        normalCompPipeline;
    vk::Pipeline        tangentCompPipeline;
    vk::Pipeline        texcoordsCompPipeline;
    vk::Pipeline        colorCompPipeline;

    bool hasBeenFlashed = false;

    vk::Device device;
    vma::Allocator vma_allocator;

    class Graphics* const graphics_ptr;

    size_t indexCounter = 0;
    size_t swapIndex = 0;

    const uint32_t waveSize;
    const uint32_t maxMorphWeights = MAX_MORPH_WEIGHTS;
    const uint32_t removeCountdown = 2;
};