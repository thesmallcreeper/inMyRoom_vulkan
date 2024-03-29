#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "common/structs/Histogram.h"

struct ExposureComputePushConstants {
    uint32_t size_x;
    uint32_t size_y;
    uint32_t frame_count;

    float min_luminance_log2;
    float max_luminance_log2;
    float HDR_factor;
};

class Graphics;

class Exposure
{
public:
    Exposure(vk::Device device,
             vma::Allocator allocator,
             const Graphics* graphics_ptr,
             std::tuple<vk::Image, vk::ImageView, vk::ImageCreateInfo> images[2],
             std::pair<vk::Queue, uint32_t> queue,
             bool check_alpha = false,
             bool luminance_input = false);
    ~Exposure();

    void SetMinMaxLuminance(float min, float max) {minLuminance = min; maxLuminance = max;};
    void SetRobustness(float low, float high) {lowRobustness = low; highRobustness = high;};
    void SetT63percent(float new_t) {t63percent = new_t;}

    void CalcNextFrameValue(size_t frame_count, float time_step);
    float GetCurrectScale() const {return 1.f / currentExposure;}

    vk::ImageMemoryBarrier GetGenericImageBarrier(uint32_t image_index) const;
    void ObtainImageOwnership(vk::CommandBuffer command_buffer,
                              uint32_t image_index,
                              vk::ImageLayout source_image_layout,
                              uint32_t source_family_index) const;
    void TransferImageOwnership(vk::CommandBuffer command_buffer,
                                uint32_t image_index,
                                vk::ImageLayout dst_image_layout,
                                uint32_t dst_family_index) const;

    void RecordFrameHistogram(vk::CommandBuffer command_buffer,
                              uint32_t image_index,
                              uint32_t frames_sum = 1,
                              float HDR_factor = 1.f) const;

private:
    void InitBuffers();
    void InitDescriptors();
    void InitPipeline();

    float CalculateHistogram(Histogram histogram) const;

private:
    vk::Pipeline            histogramPipeline;
    vk::PipelineLayout      histogramPipelineLayout;

    vk::Buffer              resultBuffer;
    vma::Allocation         resultAllocation;
    vma::AllocationInfo     resultAllocInfo;
    size_t                  resultBufferRangeSize;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       descriptorSets[3];
    vk::DescriptorSetLayout descriptorSetLayout;

    std::tuple<vk::Image, vk::ImageView, vk::ImageCreateInfo> images[2];

    float                   currentExposure = 1.5e3f;

    float                   minLuminance = 0.4e3;
    float                   maxLuminance = 2.0e4;

    float                   lowRobustness = 0.85f;
    float                   highRobustness = 0.98f;

    float                   t63percent = 0.8f;

    const bool              checkAlpha;
    const bool              luminanceInput;

    vk::Device              device;
    vma::Allocator          vma_allocator;
    const Graphics*         graphics_ptr;
    uint32_t                queue_family_index;

    uint32_t                waveSize;
    uint32_t                localSize;

    size_t                  frameCount = 0;
};