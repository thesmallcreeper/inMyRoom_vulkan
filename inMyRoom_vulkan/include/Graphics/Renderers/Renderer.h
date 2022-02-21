#pragma once
#include "Graphics/RendererBase.h"

#include "Geometry/FrustumCulling.h"

class Renderer
    : public RendererBase
{
#include "common/structs/PrimitiveInstanceParameters.h"
public:
    Renderer(class Graphics* in_graphics_ptr,
             vk::Device in_device,
             vma::Allocator in_vma_allocator);
    ~Renderer() override;

    void DrawFrame(ViewportFrustum viewport,
                   std::vector<ModelMatrices>&& matrices,
                   std::vector<DrawInfo>&& draw_infos) override;

private:
    void InitBuffers();
    void InitTLASes();
    void InitDescriptors();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPrimitivesSet();
    void InitShadePipeline();
    void InitToneMapPipeline();

    void RecordCommandBuffer(vk::CommandBuffer command_buffer,
                             uint32_t buffer_index,
                             uint32_t swapchain_index,
                             const FrustumCulling& frustum_culling);

    std::vector<PrimitiveInstanceParameters> CreatePrimitivesInstanceParameters(std::vector<DrawInfo>& draw_infos) const;
    std::vector<vk::AccelerationStructureInstanceKHR> CreateTLASinstances(const std::vector<DrawInfo>& draw_infos,
                                                                          const std::vector<ModelMatrices>& matrices,
                                                                          uint32_t buffer_index) const;

private:
    ViewportFrustum         viewport;
    std::vector<ModelMatrices> matrices;
    std::vector<DrawInfo>   draw_infos;

    std::vector<PrimitiveInstanceParameters> primitive_instance_parameters;
    std::vector<vk::AccelerationStructureInstanceKHR> TLAS_instances;

    std::pair<vk::Queue, uint32_t> graphicsQueue;
    size_t                  frameCount = 0;
    size_t                  viewportFreezedFrameCount = 0;
    size_t                  viewportInRowFreezedFrameCount = 0;

    vk::Buffer              primitivesInstanceBuffer;
    vma::Allocation         primitivesInstanceAllocation;
    vma::AllocationInfo     primitivesInstanceAllocInfo;
    size_t                  primitivesInstanceBufferHalfsize;

    vk::Buffer              fullscreenBuffer;
    vma::Allocation         fullscreenAllocation;
    vma::AllocationInfo     fullscreenAllocInfo;
    size_t                  fullscreenBufferHalfsize;

    vk::Buffer              TLASesInstancesBuffer;
    vma::Allocation         TLASesInstancesAllocation;
    vma::AllocationInfo     TLASesInstancesAllocInfo;
    size_t                  TLASesInstancesHalfSize;

    vk::Buffer              TLASesBuffer;
    vma::Allocation         TLASesAllocation;
    vk::AccelerationStructureKHR TLASesHandles[2];
    uint64_t                TLASesDeviceAddresses[2];
    size_t                  TLASesHalfSize;

    vk::Buffer              TLASbuildScratchBuffer;
    vma::Allocation         TLASbuildScratchAllocation;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       rendererDescriptorSets[2];
    vk::DescriptorSetLayout rendererDescriptorSetLayout;
    vk::DescriptorSet       toneMapDescriptorSets[2];
    vk::DescriptorSetLayout toneMapDescriptorSetLayout;

    vk::Image               depthImage;
    vma::Allocation         depthAllocation;
    vk::ImageCreateInfo     depthImageCreateInfo;
    vk::ImageView           depthImageView;

    vk::Image               visibilityImage;
    vma::Allocation         visibilityAllocation;
    vk::ImageCreateInfo     visibilityImageCreateInfo;
    vk::ImageView           visibilityImageView;

    vk::Image               photometricResultImage;
    vma::Allocation         photometricResultAllocation;
    vk::ImageCreateInfo     photometricResultImageCreateInfo;
    vk::ImageView           photometricResultImageView;

    // TODO float4 image

    vk::RenderPass          renderpass;

    std::vector<vk::Framebuffer> framebuffers;

    vk::Semaphore           readyForPresentSemaphores[3];
    vk::Semaphore           presentImageAvailableSemaphores[3];
    vk::Semaphore           hostWriteFinishTimelineSemaphore;
    vk::Semaphore           submitFinishTimelineSemaphore;

    vk::CommandPool         commandPool;
    vk::CommandBuffer       commandBuffers[3];

    std::vector<vk::Pipeline>       primitivesPipelines;
    std::vector<vk::PipelineLayout> primitivesPipelineLayouts;

    vk::Pipeline            fullscreenPipeline;
    vk::PipelineLayout      fullscreenPipelineLayout;

    vk::Pipeline            toneMapPipeline;
    vk::PipelineLayout      toneMapPipelineLayout;
};