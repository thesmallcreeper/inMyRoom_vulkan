#pragma once

#include "Graphics/Meshes/PrimitivesOfMeshes.h"
#include "Graphics/PipelinesFactory.h"
#include "Graphics/ShadersSetsFamiliesCache.h"
#include "ECS/ECStypes.h"

#define MAX_MORPH_WEIGHTS 8

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

struct DynamicPrimitiveInfo {
    size_t primitiveIndex           = -1;

    VkDeviceSize positionOffset     = -1;

    VkDeviceSize normalOffset       = -1;

    VkDeviceSize tangentOffset      = -1;

    int texcoordsCount              =  0;
    VkDeviceSize texcoordsOffset    = -1;

    VkDeviceSize colorOffset        = -1;

    vk::Buffer buffer;
    size_t halfSize                 =  0;
    size_t descriptorIndex          = -1;
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
    std::array<float, MAX_MORPH_WEIGHTS> morph_weights;
};

class DynamicMeshes
{
public:
    DynamicMeshes(class Graphics* in_graphics_ptr,
                  vk::Device device, vma::Allocator vma_allocator,
                  size_t max_dynamicMeshes);
    ~DynamicMeshes();

    void FlashDevice();

    const std::vector<DynamicPrimitiveInfo>& GetDynamicPrimitivesInfo(size_t index) const;
    size_t AddDynamicMesh(const std::vector<size_t>& primitives_info_indices);
    void RemoveDynamicMeshSafe(size_t index);

    void RecordTransformations(vk::CommandBuffer command_buffer,
                               const std::vector<DrawInfo>& draw_infos);

    vk::DescriptorSet GetDescriptorSet() {return descriptorSets[swapsCounter % 2];}
    vk::DescriptorSetLayout GetDescriptorLayout() {return descriptorSetLayout;}
    void SwapDescriptorSet();
    void CompleteRemovesSafe();
private:
    std::unordered_map<size_t, std::vector<DynamicPrimitiveInfo>> indexToDynamicPrimitivesInfos_umap;
    std::unordered_map<vk::Buffer, vma::Allocation> bufferToVMAallocation_umap;

    std::vector<size_t> indicesToBeRemovedInNextTwoRemoves;
    std::vector<size_t> indicesToBeRemovedInNextRemoves;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       descriptorSets[2];
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
    size_t swapsCounter = 0;

    const uint32_t maxMorphWeights = MAX_MORPH_WEIGHTS;
    const uint32_t waveSize = 32;
};