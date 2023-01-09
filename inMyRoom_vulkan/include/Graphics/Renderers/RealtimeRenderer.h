#pragma once
#include "Graphics/RendererBase.h"

#include "Graphics/Lights.h"
#include "Graphics/TLASbuilder.h"
#include "Graphics/NRDintegration.h"
#include "Graphics/Exposure.h"

#include "Geometry/FrustumCulling.h"

class RealtimeRenderer
    : public RendererBase
{
public:
    RealtimeRenderer(class Graphics* in_graphics_ptr,
                    vk::Device in_device,
                    vma::Allocator in_vma_allocator,
                    nrd::Method in_NRDmethod);
    ~RealtimeRenderer() override;

    void DrawFrame(const ViewportFrustum& viewport,
                   std::vector<ModelMatrices>&& matrices,
                   std::vector<LightInfo>&& light_infos,
                   std::vector<DrawInfo>&& draw_infos) override;

private:
    void InitBuffers();
    void InitImages();
    void InitNRD();
    void InitExposure();
    void InitTLAS();
    void InitDescriptors();
    void InitRenderpasses();
    void InitFramebuffers();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPrimitivesSet();
    void InitLightsDrawPipeline();
    void InitPathTracePipeline();
    void InitResolveComputePipeline();
    void InitMorphologicalAAcomputePipeline();

    void RecordGraphicsCommandBuffer(vk::CommandBuffer command_buffer,
                                     uint32_t swapchain_index,
                                     const FrustumCulling& frustum_culling);
    void WriteInitHostBuffers() const;
    void AssortDrawInfos();
    void BindMAAimages(uint32_t frame_index, uint32_t swapchain_index);
    void PrepareNRDsettings();

private:
    std::pair<vk::Queue, uint32_t> graphicsQueue;
    std::pair<vk::Queue, uint32_t> meshComputeQueue;
    std::pair<vk::Queue, uint32_t> exposureComputeQueue;

    size_t                  frameCount = 0;
    LightsIndicesRange      coneLightsIndicesRange;

    std::unique_ptr<NRDintegration> NRDintegration_uptr;
    nrd::ReblurSettings NRD_reBLURsettings = {};
    nrd::RelaxDiffuseSpecularSettings NRD_reLAXsettings = {};
    nrd::CommonSettings NRD_commonSettings = {};
    nrd::Method NRDmethod;

    std::vector<PrimitiveInstanceParameters> primitive_instance_parameters;
    std::vector<vk::AccelerationStructureInstanceKHR> TLAS_instances;

    std::vector<DrawInfo>   drawStaticMeshInfos;
    std::vector<DrawInfo>   drawDynamicMeshInfos;
    std::vector<DrawInfo>   drawLocalLightSources;
    std::vector<DrawInfo>   drawDirectionalLightSources;
    ViewportFrustum         prevFrameViewport;

    std::vector<vk::Pipeline>       primitivesPipelines;
    std::vector<vk::PipelineLayout> primitivesPipelineLayouts;
    vk::Pipeline            pathTracePipeline;
    vk::PipelineLayout      pathTracePipelineLayout;
    vk::Pipeline            lightDrawPipeline;
    vk::PipelineLayout      lightDrawPipelineLayout;
    vk::Pipeline            resolveCompPipeline;
    vk::PipelineLayout      resolveCompPipelineLayout;
    vk::Pipeline            morphologicalAAcompPipeline;
    vk::PipelineLayout      morphologicalAAcompPipelineLayout;

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

    vk::Image               morphologicalMaskImage;
    vma::Allocation         morphologicalMaskAllocation;
    vk::ImageView           morphologicalMaskImageView;
    vk::ImageCreateInfo     morphologicalMaskImageCreateInfo;

    vk::Image               diffuseDistanceImage;
    vma::Allocation         diffuseDistanceAllocation;
    vk::ImageView           diffuseDistanceImageView;
    vk::ImageCreateInfo     diffuseDistanceImageCreateInfo;

    vk::Image               denoisedDiffuseDistanceImage;
    vma::Allocation         denoisedDiffuseDistanceAllocation;
    vk::ImageView           denoisedDiffuseDistanceImageView;
    vk::ImageCreateInfo     denoisedDiffuseDistanceImageCreateInfo;

    vk::Image               specularDistanceImage;
    vma::Allocation         specularDistanceAllocation;
    vk::ImageView           specularDistanceImageView;
    vk::ImageCreateInfo     specularDistanceImageCreateInfo;

    vk::Image               denoisedSpecularDistanceImage;
    vma::Allocation         denoisedSpecularDistanceAllocation;
    vk::ImageView           denoisedSpecularDistanceImageView;
    vk::ImageCreateInfo     denoisedSpecularDistanceImageCreateInfo;

    vk::Image               normalRoughnessImage;
    vma::Allocation         normalRoughnessAllocation;
    vk::ImageView           normalRoughnessImageView;
    vk::ImageCreateInfo     normalRoughnessImageCreateInfo;

    vk::Image               colorMetalnessImage;
    vma::Allocation         colorMetalnessAllocation;
    vk::ImageView           colorMetalnessImageView;
    vk::ImageCreateInfo     colorMetalnessImageCreateInfo;

    vk::Image               motionImage;
    vma::Allocation         motionAllocation;
    vk::ImageView           motionImageView;
    vk::ImageCreateInfo     motionImageCreateInfo;

    vk::Image               linearViewZImage;
    vma::Allocation         linearViewZAllocation;
    vk::ImageView           linearViewZImageView;
    vk::ImageCreateInfo     linearViewZImageCreateInfo;

    vk::Image               lightSourcesPassImage;
    vma::Allocation         lightSourcesPassAllocation;
    vk::ImageView           lightSourcesPassImageView;
    vk::ImageCreateInfo     lightSourcesPassImageCreateInfo;

    vk::Image               resolveResultImage;
    vma::Allocation         resolveResultAllocation;
    vk::ImageView           resolveResultImageView;
    vk::ImageCreateInfo     resolveResultImageCreateInfo;

    vk::Image               luminanceImages[2];
    vma::Allocation         luminanceAllocations[2];
    vk::ImageView           luminanceImageViews[2];
    vk::ImageCreateInfo     luminanceImageCreateInfo;

    std::unique_ptr<TLASbuilder> TLASbuilder_uptr;
    std::unique_ptr<Exposure> exposure_uptr;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       hostDescriptorSets[3];
    vk::DescriptorSetLayout hostDescriptorSetLayout;
    vk::DescriptorSet       pathTraceDescriptorSet;
    vk::DescriptorSetLayout pathTraceDescriptorSetLayout;
    vk::DescriptorSet       resolveDescriptorSets[2];
    vk::DescriptorSetLayout resolveDescriptorSetLayout;
    vk::DescriptorSet       morphologicalAAdescriptorSets[3];
    vk::DescriptorSetLayout morphologicalAAdescriptorSetsLayout;

    const bool useMorphologicalAA = true;
    const vk::SampleCountFlagBits MAAsamplesCount = vk::SampleCountFlagBits::e8;

    const float FP16factor = 0.5e3f;
    const uint32_t comp_dim_size = 16;
    const uint32_t visibilityBufferTriangleBits = 20;
};