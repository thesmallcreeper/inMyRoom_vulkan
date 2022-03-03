#pragma once
#include "Graphics/RendererBase.h"

#include "Graphics/TLASinstance.h"
#include "Geometry/FrustumCulling.h"

enum class ViewportFreezeStates {
    ready = 0,
    next_frame_freeze = 1,
    frozen = 2,
    next_frame_unfreeze = 3,
};

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
    void InitDescriptors();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPrimitivesSet();
    void InitShadePipeline();
    void InitToneMapPipeline();

    void RecordGraphicsCommandBuffer(vk::CommandBuffer command_buffer,
                                     uint32_t freezable_host_buffer_index,
                                     uint32_t freezable_device_buffer_index,
                                     uint32_t device_buffer_index,
                                     uint32_t swapchain_index,
                                     const FrustumCulling& frustum_culling);

    std::vector<PrimitiveInstanceParameters> CreatePrimitivesInstanceParameters();

    void WriteInitHostBuffers(uint32_t buffer_index) const;
private:
    ViewportFrustum         viewport;
    std::vector<ModelMatrices> matrices;
    std::vector<DrawInfo>   drawInfos;
    std::vector<DrawInfo>   drawDynamicMeshInfos;

    std::vector<PrimitiveInstanceParameters> primitive_instance_parameters;
    std::vector<vk::AccelerationStructureInstanceKHR> TLAS_instances;

    std::pair<vk::Queue, uint32_t> graphicsQueue;
    std::pair<vk::Queue, uint32_t> computeQueue;
    size_t                  frameCount = 0;
    size_t                  viewportFreezedFrameCount = 0;
    size_t                  viewportInRowFreezedFrameCount = 0;

    vk::Buffer              primitivesInstanceBuffer;
    vma::Allocation         primitivesInstanceAllocation;
    vma::AllocationInfo     primitivesInstanceAllocInfo;
    size_t                  primitivesInstanceBufferPartSize;

    vk::Buffer              fullscreenBuffer;
    vma::Allocation         fullscreenAllocation;
    vma::AllocationInfo     fullscreenAllocInfo;
    size_t                  fullscreenBufferPartSize;

    std::unique_ptr<TLASinstance> TLASinstance_uptr;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       hostDescriptorSets[3];
    vk::DescriptorSetLayout hostDescriptorSetLayout;
    vk::DescriptorSet       rendererDescriptorSets[2];
    vk::DescriptorSetLayout rendererDescriptorSetLayout;


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

    vk::RenderPass          renderpass;

    std::vector<vk::Framebuffer> framebuffers;

    vk::Semaphore           readyForPresentSemaphores[3];
    vk::Semaphore           presentImageAvailableSemaphores[3];
    vk::Semaphore           transformsFinishTimelineSemaphore;
    vk::Semaphore           xLASupdateFinishTimelineSemaphore;
    vk::Semaphore           graphicsFinishTimelineSemaphore;
    vk::Semaphore           commandBufferFinishTimelineSemaphore;

    vk::CommandPool         graphicsCommandPool;
    vk::CommandBuffer       graphicsCommandBuffers[3];

    vk::CommandPool         computeCommandPool;
    vk::CommandBuffer       transformCommandBuffers[3];
    vk::CommandBuffer       xLASCommandBuffers[3];

    std::vector<vk::Pipeline>       primitivesPipelines;
    std::vector<vk::PipelineLayout> primitivesPipelineLayouts;

    vk::Pipeline            fullscreenPipeline;
    vk::PipelineLayout      fullscreenPipelineLayout;

    vk::Pipeline            toneMapPipeline;
    vk::PipelineLayout      toneMapPipelineLayout;

    ViewportFreezeStates    viewportFreezeState = ViewportFreezeStates::ready;
};