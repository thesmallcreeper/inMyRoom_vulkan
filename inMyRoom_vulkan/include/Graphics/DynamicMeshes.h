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
        OBB dynamicPrimitiveOBB         = OBB::EmptyOBB();

        size_t primitiveIndex           = -1;

        VkDeviceSize positionByteOffset = -1;

        VkDeviceSize normalByteOffset   = -1;

        VkDeviceSize tangentByteOffset  = -1;

        int texcoordsCount              =  0;
        VkDeviceSize texcoordsByteOffset= -1;

        VkDeviceSize colorByteOffset    = -1;
    };

    std::vector<DynamicPrimitiveInfo> dynamicPrimitives;
    size_t meshIndex                = -1;

    vk::Buffer buffer;
    vma::Allocation allocation;
    size_t rangeSize                =  0;
    size_t descriptorIndexOffset    = -1;

    bool hasDynamicShape            = false;

    // BLAS
    vk::AccelerationStructureKHR BLASesHandles[2];
    uint64_t BLASesDeviceAddresses[2] = {0, 0};
    vk::Buffer BLASesBuffer;
    vma::Allocation BLASesAllocation;
    size_t BLASesHalfSize           = 0;

    vk::Buffer updateScratchBuffer;
    vma::Allocation updateScratchAllocation;

    // AABB
    vk::Buffer AABBsBuffer;
    vma::Allocation AABBsAllocation;
    vma::AllocationInfo AABBsAllocInfo;
    size_t AABBsBufferRangeSize;

    bool shouldBeDeleted            = false;
    size_t lastUpdateFrameIndex     = -1;
    size_t inRowUpdatedFrames       = 0;
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
    uint32_t AABBresultOffset = -1;
    std::array<float, MAX_MORPH_WEIGHTS> morph_weights = {};
};

class DynamicMeshes
{
public:
    DynamicMeshes(class Graphics* in_graphics_ptr,
                  vk::Device device, vma::Allocator vma_allocator,
                  size_t max_dynamicMeshes);
    ~DynamicMeshes();

    void FlashDevice(std::pair<vk::Queue, uint32_t> queue);

    const DynamicMeshInfo& GetDynamicMeshInfo(size_t index) const;
    size_t AddDynamicMesh(size_t mesh_index);
    void RemoveDynamicMeshSafe(size_t index);

    std::vector<vk::BufferMemoryBarrier> GetGenericTransformRangesBarriers(const std::vector<DrawInfo> &draw_infos,
                                                                           uint32_t buffer_index) const;
    std::vector<vk::BufferMemoryBarrier> GetGenericBLASrangesBarriers(const std::vector<DrawInfo> &draw_infos,
                                                                      uint32_t buffer_index) const;

    void ObtainTransformRanges(vk::CommandBuffer command_buffer,
                               const std::vector<DrawInfo>& draw_infos,
                               uint32_t source_family_index) const;
    void RecordTransformations(vk::CommandBuffer command_buffer,
                               const std::vector<DrawInfo>& draw_infos);

    // Transform and BLAS update sync with semaphore!

    void ObtainBLASranges(vk::CommandBuffer command_buffer,
                          const std::vector<DrawInfo>& draw_infos,
                          uint32_t source_family_index) const;
    void RecordBLASupdate(vk::CommandBuffer command_buffer,
                          const std::vector<DrawInfo>& draw_infos) const;

    // After BLAS update, the TLAS update should follow!

    void TransferTransformAndBLASranges(vk::CommandBuffer command_buffer,
                                        const std::vector<DrawInfo>& draw_infos,
                                        uint32_t dst_family_index) const;


    vk::DescriptorSet GetDescriptorSet() const {return verticesDescriptorSets[frameIndex % 3];}
    vk::DescriptorSetLayout GetDescriptorLayout() const {return verticesDescriptorSetLayout;}
    void PrepareNewFrame(size_t frame_index);
    void CompleteRemovesSafe();

private:
    DynamicMeshInfo& GetDynamicMeshInfoPriv(size_t index);

    vk::DescriptorSet GetAABBsDescriptorSet() const {return AABBsAndScratchDescriptorSets[frameIndex % 3];}
    vk::DescriptorSetLayout GetAABBsDescriptorLayout() const {return AABBsAndScratchDescriptorSetLayout;}

    void UpdateHostAABBs();
    void SwapDescriptorSets();

private:
    const size_t max_dynamicMeshes;

    std::unordered_map<size_t, DynamicMeshInfo> indexToDynamicMeshInfo_umap;
    std::vector<std::pair<size_t, uint32_t>> indexToBeRemovedCountdown;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       verticesDescriptorSets[3];
    vk::DescriptorSetLayout verticesDescriptorSetLayout;
    vk::DescriptorSet       AABBsAndScratchDescriptorSets[3];
    vk::DescriptorSetLayout AABBsAndScratchDescriptorSetLayout;

    vk::PipelineLayout  position_computeLayout;
    vk::Pipeline        positionCompPipeline;
    vk::PipelineLayout  generic_computeLayout;
    vk::Pipeline        normalCompPipeline;
    vk::Pipeline        tangentCompPipeline;
    vk::Pipeline        texcoordsCompPipeline;
    vk::Pipeline        colorCompPipeline;

    vk::Buffer              AABBinitDataBuffer;
    vma::Allocation         AABBinitDataAllocation;

    bool hasBeenFlashed = false;

    vk::Device device;
    vma::Allocator vma_allocator;

    class Graphics* const graphics_ptr;
    uint32_t queue_family_index;

    size_t indexCounter = 0;
    size_t frameIndex = 0;

    const uint32_t waveSize;
    const uint32_t accumulateLocalSize;
    const uint32_t maxMorphWeights = MAX_MORPH_WEIGHTS;
    const uint32_t removeCountdown = 2;
};