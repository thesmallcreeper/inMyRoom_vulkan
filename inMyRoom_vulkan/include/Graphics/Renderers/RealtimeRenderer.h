#pragma once
#include "Graphics/RendererBase.h"

#include "Graphics/Lights.h"
#include "Graphics/TLASbuilder.h"

#include "Geometry/FrustumCulling.h"

class RealtimeRenderer
    : public RendererBase
{
public:
    RealtimeRenderer(class Graphics* in_graphics_ptr,
                    vk::Device in_device,
                    vma::Allocator in_vma_allocator);
    ~RealtimeRenderer() override;

    void DrawFrame(const ViewportFrustum& viewport,
                   std::vector<ModelMatrices>&& matrices,
                   std::vector<LightInfo>&& light_infos,
                   std::vector<DrawInfo>&& draw_infos) override;

private:
    void InitBuffers();
    void InitImages();
    // void InitExposure();
    void InitTLAS();
    void InitDescriptors();
    void InitRenderpasses();
    void InitFramebuffers();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPrimitivesSet();
    void InitPathTracePipeline();
    void InitResolveComputePipeline();

    void RecordGraphicsCommandBuffer(vk::CommandBuffer command_buffer,
                                     uint32_t swapchain_index,
                                     const FrustumCulling& frustum_culling);
    void WriteInitHostBuffers() const;
    void AssortDrawInfos();
    void BindSwapImage(uint32_t frame_index, uint32_t swapchain_index);

private:
    std::pair<vk::Queue, uint32_t> graphicsQueue;
    std::pair<vk::Queue, uint32_t> meshComputeQueue;
    std::pair<vk::Queue, uint32_t> exposureComputeQueue;

    size_t                  frameCount = 0;
    LightsIndicesRange      coneLightsIndicesRange;

    std::vector<PrimitiveInstanceParameters> primitive_instance_parameters;
    std::vector<vk::AccelerationStructureInstanceKHR> TLAS_instances;

    std::vector<DrawInfo>   drawStaticMeshInfos;
    std::vector<DrawInfo>   drawDynamicMeshInfos;
    std::vector<DrawInfo>   drawLocalLightSources;
    std::vector<DrawInfo>   drawDirectionalLightSources;

    std::vector<vk::Pipeline>       primitivesPipelines;
    std::vector<vk::PipelineLayout> primitivesPipelineLayouts;
    vk::Pipeline            pathTracePipeline;
    vk::PipelineLayout      pathTracePipelineLayout;
    vk::Pipeline            resolveCompPipeline;
    vk::PipelineLayout      resolveCompPipelineLayout;

    vk::CommandPool         graphicsCommandPool;
    vk::CommandBuffer       graphicsCommandBuffers[3];

    vk::CommandPool         computeCommandPool;
    vk::CommandBuffer       transformCommandBuffers[3];
    vk::CommandBuffer       xLASCommandBuffers[3];
    vk::CommandBuffer       exposureCommandBuffers[3];

    vk::Semaphore           readyForPresentSemaphores[3];
    vk::Semaphore           presentImageAvailableSemaphores[3];
    vk::Semaphore           transformsFinishTimelineSemaphore;
    vk::Semaphore           xLASupdateFinishTimelineSemaphore;
    vk::Semaphore           graphicsFinishTimelineSemaphore;
    vk::Semaphore           histogramFinishTimelineSemaphore;

    vk::RenderPass          renderpass;
    vk::Framebuffer         frameBuffer;

    vk::Buffer              primitivesInstanceBuffer;
    vma::Allocation         primitivesInstanceAllocation;
    vma::AllocationInfo     primitivesInstanceAllocInfo;
    size_t                  primitivesInstanceBufferPartSize;

    vk::Buffer              fullscreenBuffer;
    vma::Allocation         fullscreenAllocation;
    vma::AllocationInfo     fullscreenAllocInfo;
    size_t                  fullscreenBufferPartSize;

    vk::Image               depthImage;
    vma::Allocation         depthAllocation;
    vk::ImageCreateInfo     depthImageCreateInfo;
    vk::ImageView           depthImageView;

    vk::Image               visibilityImage;
    vma::Allocation         visibilityAllocation;
    vk::ImageCreateInfo     visibilityImageCreateInfo;
    vk::ImageView           visibilityImageView;

    vk::Image               diffuseDistanceImage;
    vma::Allocation         diffuseDistanceAllocation;
    vk::ImageView           diffuseDistanceImageView;
    vk::ImageCreateInfo     diffuseDistanceImageCreateInfo;

    vk::Image               specularDistanceImage;
    vma::Allocation         specularDistanceAllocation;
    vk::ImageView           specularDistanceImageView;
    vk::ImageCreateInfo     specularDistanceImageCreateInfo;

    std::unique_ptr<TLASbuilder> TLASbuilder_uptr;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       hostDescriptorSets[3];
    vk::DescriptorSetLayout hostDescriptorSetLayout;
    vk::DescriptorSet       pathTraceDescriptorSet;
    vk::DescriptorSetLayout pathTraceDescriptorSetLayout;
    vk::DescriptorSet       resolveDescriptorSets[3];
    vk::DescriptorSetLayout resolveDescriptorSetLayout;

    const float FP16factor = 0.5e3f;
    const uint32_t comp_dim_size = 16;
};