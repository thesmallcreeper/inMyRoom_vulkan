#include "Graphics/Renderers/RealtimeRenderer.h"

#include "Graphics/Graphics.h"
#include "Graphics/HelperUtils.h"

RealtimeRenderer::RealtimeRenderer(Graphics *in_graphics_ptr,
                                   vk::Device in_device,
                                   vma::Allocator in_vma_allocator,
                                   nrd::Method in_NRDmethod)
        : RendererBase(in_graphics_ptr, in_device, in_vma_allocator),
          graphicsQueue(graphics_ptr->GetQueuesList().graphicsQueues[0]),
          NRDmethod(in_NRDmethod),
          useMorphologicalAA(true),
#ifdef ENABLE_ASYNC_COMPUTE
          meshComputeQueue(graphics_ptr->GetQueuesList().dedicatedComputeQueues[0]),
          exposureComputeQueue(graphics_ptr->GetQueuesList().dedicatedComputeQueues[1])
#else
          meshComputeQueue(graphics_ptr->GetQueuesList().graphicsQueues[0]),
          exposureComputeQueue(graphics_ptr->GetQueuesList().graphicsQueues[0])
#endif
{
    InitBuffers();
    InitImages();
    InitNRD();
    InitExposure();
    InitTLAS();
    InitDescriptors();
    InitRenderpasses();
    InitFramebuffers();
    InitSemaphoresAndFences();
    InitCommandBuffers();
    InitPrimitivesSet();
    InitPathTracePipeline();
    InitLightsDrawPipeline();
    InitResolveComputePipeline();
    if (useMorphologicalAA) {
        InitMorphologicalAAcomputePipeline();
    }
}

RealtimeRenderer::~RealtimeRenderer()
{
    device.waitIdle();

    device.destroy(graphicsCommandPool);
    device.destroy(computeCommandPool);

    device.destroy(readyForPresentSemaphores[0]);
    device.destroy(readyForPresentSemaphores[1]);
    device.destroy(readyForPresentSemaphores[2]);
    device.destroy(presentImageAvailableSemaphores[0]);
    device.destroy(presentImageAvailableSemaphores[1]);
    device.destroy(presentImageAvailableSemaphores[2]);
    device.destroy(graphicsFinishTimelineSemaphore);
    device.destroy(transformsFinishTimelineSemaphore);
    device.destroy(xLASupdateFinishTimelineSemaphore);
    device.destroy(histogramFinishTimelineSemaphore);

    device.destroy(frameBuffer);
    device.destroy(renderpass);

    device.destroy(descriptorPool);
    device.destroy(hostDescriptorSetLayout);
    device.destroy(pathTraceDescriptorSetLayout);
    device.destroy(resolveDescriptorSetLayout);
    device.destroy(morphologicalAAdescriptorSetsLayout);

    TLASbuilder_uptr.reset();
    NRDintegration_uptr.reset();
    exposure_uptr.reset();

    device.destroy(depthImageView);
    vma_allocator.destroyImage(depthImage, depthAllocation);

    device.destroy(visibilityImageView);
    vma_allocator.destroyImage(visibilityImage, visibilityAllocation);

    if (useMorphologicalAA) {
        device.destroy(depthResolvedImageView);
        vma_allocator.destroyImage(depthResolvedImage, depthResolvedAllocation);

        device.destroy(morphologicalMaskImageView);
        vma_allocator.destroyImage(morphologicalMaskImage, morphologicalMaskAllocation);
    }

    device.destroy(diffuseDistanceImageView);
    vma_allocator.destroyImage(diffuseDistanceImage, diffuseDistanceAllocation);

    device.destroy(denoisedDiffuseDistanceImageView);
    vma_allocator.destroyImage(denoisedDiffuseDistanceImage, denoisedDiffuseDistanceAllocation);

    device.destroy(specularDistanceImageView);
    vma_allocator.destroyImage(specularDistanceImage, specularDistanceAllocation);

    device.destroy(denoisedSpecularDistanceImageView);
    vma_allocator.destroyImage(denoisedSpecularDistanceImage, denoisedSpecularDistanceAllocation);

    device.destroy(normalRoughnessImageView);
    vma_allocator.destroyImage(normalRoughnessImage, normalRoughnessAllocation);

    device.destroy(colorMetalnessImageView);
    vma_allocator.destroyImage(colorMetalnessImage, colorMetalnessAllocation);

    device.destroy(motionImageView);
    vma_allocator.destroyImage(motionImage, motionAllocation);

    device.destroy(linearViewZImageView);
    vma_allocator.destroyImage(linearViewZImage, linearViewZAllocation);

    device.destroy(lightSourcesPassImageView);
    vma_allocator.destroyImage(lightSourcesPassImage, lightSourcesPassAllocation);

    device.destroy(resolveResultImageView);
    vma_allocator.destroyImage(resolveResultImage, resolveResultAllocation);

    device.destroy(luminanceImageViews[0]);
    vma_allocator.destroyImage(luminanceImages[0], luminanceAllocations[0]);
    device.destroy(luminanceImageViews[1]);
    vma_allocator.destroyImage(luminanceImages[1], luminanceAllocations[1]);

    vma_allocator.destroyBuffer(primitivesInstanceBuffer, primitivesInstanceAllocation);
    vma_allocator.destroyBuffer(fullscreenBuffer, fullscreenAllocation);
}

void RealtimeRenderer::InitBuffers()
{
    // primitivesInstanceBuffer
    {
        primitivesInstanceBufferPartSize = sizeof(PrimitiveInstanceParameters) * graphics_ptr->GetMaxInstancesCount();
        primitivesInstanceBufferPartSize += (primitivesInstanceBufferPartSize % 16 == 0) ? 0 : 16 - primitivesInstanceBufferPartSize % 16;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = primitivesInstanceBufferPartSize * 3;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              primitivesInstanceAllocInfo);

        assert(createBuffer_result.result == vk::Result::eSuccess);
        primitivesInstanceBuffer = createBuffer_result.value.first;
        primitivesInstanceAllocation = createBuffer_result.value.second;
    }

    // full-screen pass
    {
        fullscreenBufferPartSize = sizeof(glm::vec4) * 6 ;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = fullscreenBufferPartSize * 3;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              fullscreenAllocInfo);

        assert(createBuffer_result.result == vk::Result::eSuccess);
        fullscreenBuffer = createBuffer_result.value.first;
        fullscreenAllocation = createBuffer_result.value.second;
    }
}

void RealtimeRenderer::InitImages()
{
    // z-buffer
    {   // TODO: Change?
        depthImageCreateInfo.imageType = vk::ImageType::e2D;
        depthImageCreateInfo.format = vk::Format::eD32Sfloat;
        depthImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        depthImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        depthImageCreateInfo.extent.depth = 1;
        depthImageCreateInfo.mipLevels = 1;
        depthImageCreateInfo.arrayLayers = 1;
        depthImageCreateInfo.samples = useMorphologicalAA ? MAAsamplesCount : vk::SampleCountFlagBits::e1;
        depthImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        depthImageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depthImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        depthImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(depthImageCreateInfo, image_allocation_info).value;
        depthImage = createImage_result.first;
        depthAllocation = createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = depthImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = depthImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eDepth,
                                                  0, 1,
                                                  0, 1};

        depthImageView = device.createImageView(imageView_create_info).value;
    }

    // z-buffer resolved
    if (useMorphologicalAA) {   // TODO: Change format?
        depthResolvedImageCreateInfo.imageType = vk::ImageType::e2D;
        depthResolvedImageCreateInfo.format = vk::Format::eD32Sfloat;
        depthResolvedImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        depthResolvedImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        depthResolvedImageCreateInfo.extent.depth = 1;
        depthResolvedImageCreateInfo.mipLevels = 1;
        depthResolvedImageCreateInfo.arrayLayers = 1;
        depthResolvedImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        depthResolvedImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        depthResolvedImageCreateInfo.usage = vk::ImageUsageFlagBits::eDepthStencilAttachment;
        depthResolvedImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        depthResolvedImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(depthResolvedImageCreateInfo, image_allocation_info).value;
        depthResolvedImage = createImage_result.first;
        depthResolvedAllocation = createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = depthResolvedImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = depthResolvedImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eDepth,
                                                  0, 1,
                                                  0, 1};

        depthResolvedImageView = device.createImageView(imageView_create_info).value;
    }

    // visibility image
    {
        visibilityImageCreateInfo.imageType = vk::ImageType::e2D;
        visibilityImageCreateInfo.format = vk::Format::eR32G32Uint;
        visibilityImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        visibilityImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        visibilityImageCreateInfo.extent.depth = 1;
        visibilityImageCreateInfo.mipLevels = 1;
        visibilityImageCreateInfo.arrayLayers = 1;
        visibilityImageCreateInfo.samples = useMorphologicalAA ? MAAsamplesCount : vk::SampleCountFlagBits::e1;
        visibilityImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        visibilityImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eInputAttachment;
        visibilityImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        visibilityImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(visibilityImageCreateInfo, image_allocation_info).value;
        visibilityImage = createImage_result.first;
        visibilityAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = visibilityImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = visibilityImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        visibilityImageView = device.createImageView(imageView_create_info).value;
    }

    // morphological mask image
    if (useMorphologicalAA) {
        morphologicalMaskImageCreateInfo.imageType = vk::ImageType::e2D;
        morphologicalMaskImageCreateInfo.format = vk::Format::eR8Uint;
        morphologicalMaskImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        morphologicalMaskImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        morphologicalMaskImageCreateInfo.extent.depth = 1;
        morphologicalMaskImageCreateInfo.mipLevels = 1;
        morphologicalMaskImageCreateInfo.arrayLayers = 1;
        morphologicalMaskImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        morphologicalMaskImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        morphologicalMaskImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;
        morphologicalMaskImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        morphologicalMaskImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(morphologicalMaskImageCreateInfo, image_allocation_info).value;
        morphologicalMaskImage = createImage_result.first;
        morphologicalMaskAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = morphologicalMaskImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = morphologicalMaskImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        morphologicalMaskImageView = device.createImageView(imageView_create_info).value;
    }

    // diffuse-distance image
    {
        diffuseDistanceImageCreateInfo.imageType = vk::ImageType::e2D;
        diffuseDistanceImageCreateInfo.format = vk::Format::eR16G16B16A16Sfloat;
        diffuseDistanceImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        diffuseDistanceImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        diffuseDistanceImageCreateInfo.extent.depth = 1;
        diffuseDistanceImageCreateInfo.mipLevels = 1;
        diffuseDistanceImageCreateInfo.arrayLayers = 1;
        diffuseDistanceImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        diffuseDistanceImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        diffuseDistanceImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |  vk::ImageUsageFlagBits::eSampled;
        diffuseDistanceImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        diffuseDistanceImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(diffuseDistanceImageCreateInfo, image_allocation_info).value;
        diffuseDistanceImage = createImage_result.first;
        diffuseDistanceAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = diffuseDistanceImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = diffuseDistanceImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        diffuseDistanceImageView = device.createImageView(imageView_create_info).value;
    }

    // denoised diffuse-distance image
    {
        denoisedDiffuseDistanceImageCreateInfo.imageType = vk::ImageType::e2D;
        denoisedDiffuseDistanceImageCreateInfo.format = vk::Format::eR16G16B16A16Sfloat;
        denoisedDiffuseDistanceImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        denoisedDiffuseDistanceImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        denoisedDiffuseDistanceImageCreateInfo.extent.depth = 1;
        denoisedDiffuseDistanceImageCreateInfo.mipLevels = 1;
        denoisedDiffuseDistanceImageCreateInfo.arrayLayers = 1;
        denoisedDiffuseDistanceImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        denoisedDiffuseDistanceImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        denoisedDiffuseDistanceImageCreateInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
        denoisedDiffuseDistanceImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        denoisedDiffuseDistanceImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(denoisedDiffuseDistanceImageCreateInfo, image_allocation_info).value;
        denoisedDiffuseDistanceImage = createImage_result.first;
        denoisedDiffuseDistanceAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = denoisedDiffuseDistanceImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = denoisedDiffuseDistanceImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        denoisedDiffuseDistanceImageView = device.createImageView(imageView_create_info).value;
    }

    // specular-distance image
    {
        specularDistanceImageCreateInfo.imageType = vk::ImageType::e2D;
        specularDistanceImageCreateInfo.format = vk::Format::eR16G16B16A16Sfloat;
        specularDistanceImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        specularDistanceImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        specularDistanceImageCreateInfo.extent.depth = 1;
        specularDistanceImageCreateInfo.mipLevels = 1;
        specularDistanceImageCreateInfo.arrayLayers = 1;
        specularDistanceImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        specularDistanceImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        specularDistanceImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |  vk::ImageUsageFlagBits::eSampled;
        specularDistanceImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        specularDistanceImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(specularDistanceImageCreateInfo, image_allocation_info).value;
        specularDistanceImage = createImage_result.first;
        specularDistanceAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = specularDistanceImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = specularDistanceImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        specularDistanceImageView = device.createImageView(imageView_create_info).value;
    }

    // denoised specular-distance image
    {
        denoisedSpecularDistanceImageCreateInfo.imageType = vk::ImageType::e2D;
        denoisedSpecularDistanceImageCreateInfo.format = vk::Format::eR16G16B16A16Sfloat;
        denoisedSpecularDistanceImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        denoisedSpecularDistanceImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        denoisedSpecularDistanceImageCreateInfo.extent.depth = 1;
        denoisedSpecularDistanceImageCreateInfo.mipLevels = 1;
        denoisedSpecularDistanceImageCreateInfo.arrayLayers = 1;
        denoisedSpecularDistanceImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        denoisedSpecularDistanceImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        denoisedSpecularDistanceImageCreateInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
        denoisedSpecularDistanceImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        denoisedSpecularDistanceImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(denoisedSpecularDistanceImageCreateInfo, image_allocation_info).value;
        denoisedSpecularDistanceImage = createImage_result.first;
        denoisedSpecularDistanceAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = denoisedSpecularDistanceImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = denoisedSpecularDistanceImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        denoisedSpecularDistanceImageView = device.createImageView(imageView_create_info).value;
    }

    // normalRoughness image
    {
        normalRoughnessImageCreateInfo.imageType = vk::ImageType::e2D;
        normalRoughnessImageCreateInfo.format = vk::Format::eA2B10G10R10UnormPack32;
        normalRoughnessImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        normalRoughnessImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        normalRoughnessImageCreateInfo.extent.depth = 1;
        normalRoughnessImageCreateInfo.mipLevels = 1;
        normalRoughnessImageCreateInfo.arrayLayers = 1;
        normalRoughnessImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        normalRoughnessImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        normalRoughnessImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
        normalRoughnessImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        normalRoughnessImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(normalRoughnessImageCreateInfo, image_allocation_info).value;
        normalRoughnessImage = createImage_result.first;
        normalRoughnessAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = normalRoughnessImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = normalRoughnessImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        normalRoughnessImageView = device.createImageView(imageView_create_info).value;
    }

    // colorMetalness image
    {
        colorMetalnessImageCreateInfo.imageType = vk::ImageType::e2D;
        colorMetalnessImageCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
        colorMetalnessImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        colorMetalnessImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        colorMetalnessImageCreateInfo.extent.depth = 1;
        colorMetalnessImageCreateInfo.mipLevels = 1;
        colorMetalnessImageCreateInfo.arrayLayers = 1;
        colorMetalnessImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        colorMetalnessImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        colorMetalnessImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage;
        colorMetalnessImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        colorMetalnessImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(colorMetalnessImageCreateInfo, image_allocation_info).value;
        colorMetalnessImage = createImage_result.first;
        colorMetalnessAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = colorMetalnessImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = colorMetalnessImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        colorMetalnessImageView = device.createImageView(imageView_create_info).value;
    }

    // motion image
    {
        motionImageCreateInfo.imageType = vk::ImageType::e2D;
        motionImageCreateInfo.format = vk::Format::eR16G16B16A16Sfloat;
        motionImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        motionImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        motionImageCreateInfo.extent.depth = 1;
        motionImageCreateInfo.mipLevels = 1;
        motionImageCreateInfo.arrayLayers = 1;
        motionImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        motionImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        motionImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
        motionImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        motionImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(motionImageCreateInfo, image_allocation_info).value;
        motionImage = createImage_result.first;
        motionAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = motionImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = motionImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        motionImageView = device.createImageView(imageView_create_info).value;
    }

    // linear viewZ image
    {
        linearViewZImageCreateInfo.imageType = vk::ImageType::e2D;
        linearViewZImageCreateInfo.format = vk::Format::eR16Sfloat;
        linearViewZImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        linearViewZImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        linearViewZImageCreateInfo.extent.depth = 1;
        linearViewZImageCreateInfo.mipLevels = 1;
        linearViewZImageCreateInfo.arrayLayers = 1;
        linearViewZImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        linearViewZImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        linearViewZImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eStorage |  vk::ImageUsageFlagBits::eSampled;
        linearViewZImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        linearViewZImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(linearViewZImageCreateInfo, image_allocation_info).value;
        linearViewZImage = createImage_result.first;
        linearViewZAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = linearViewZImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = linearViewZImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        linearViewZImageView = device.createImageView(imageView_create_info).value;
    }

    // light sources pass
    {
        lightSourcesPassImageCreateInfo.imageType = vk::ImageType::e2D;
        lightSourcesPassImageCreateInfo.format = vk::Format::eR16G16B16A16Sfloat;
        lightSourcesPassImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        lightSourcesPassImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        lightSourcesPassImageCreateInfo.extent.depth = 1;
        lightSourcesPassImageCreateInfo.mipLevels = 1;
        lightSourcesPassImageCreateInfo.arrayLayers = 1;
        lightSourcesPassImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        lightSourcesPassImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        lightSourcesPassImageCreateInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eColorAttachment;
        lightSourcesPassImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        lightSourcesPassImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(lightSourcesPassImageCreateInfo, image_allocation_info).value;
        lightSourcesPassImage = createImage_result.first;
        lightSourcesPassAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = lightSourcesPassImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = lightSourcesPassImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        lightSourcesPassImageView = device.createImageView(imageView_create_info).value;
    }

    // resolve result image
    {
        resolveResultImageCreateInfo.imageType = vk::ImageType::e2D;
        resolveResultImageCreateInfo.format = graphics_ptr->GetSwapchainCreateInfo().imageFormat;
        resolveResultImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        resolveResultImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        resolveResultImageCreateInfo.extent.depth = 1;
        resolveResultImageCreateInfo.mipLevels = 1;
        resolveResultImageCreateInfo.arrayLayers = 1;
        resolveResultImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        resolveResultImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        resolveResultImageCreateInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eTransferSrc;
        resolveResultImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        resolveResultImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createImage_result = vma_allocator.createImage(resolveResultImageCreateInfo, image_allocation_info).value;
        resolveResultImage = createImage_result.first;
        resolveResultAllocation= createImage_result.second;

        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = resolveResultImage;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = resolveResultImageCreateInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  0, 1,
                                                  0, 1};

        resolveResultImageView = device.createImageView(imageView_create_info).value;

        // Initialize
        OneShotCommandBuffer one_shot_command_buffer(device);
        vk::CommandBuffer command_buffer = one_shot_command_buffer.BeginCommandRecord(graphicsQueue);

        vk::ImageMemoryBarrier init_image_barrier;
        init_image_barrier.image = resolveResultImage;
        init_image_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        init_image_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        init_image_barrier.oldLayout = vk::ImageLayout::eUndefined;
        init_image_barrier.newLayout = vk::ImageLayout::eGeneral;
        init_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        init_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        init_image_barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                               0, 1,
                                               0, 1};

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                       vk::PipelineStageFlagBits::eAllCommands,
                                       vk::DependencyFlagBits::eByRegion,
                                       0, nullptr,
                                       0, nullptr,
                                       1, &init_image_barrier);

        one_shot_command_buffer.EndAndSubmitCommands();
    }

    // luminance images
    {
        luminanceImageCreateInfo.imageType = vk::ImageType::e2D;
        luminanceImageCreateInfo.format = vk::Format::eR16Sfloat;
        luminanceImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        luminanceImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        luminanceImageCreateInfo.extent.depth = 1;
        luminanceImageCreateInfo.mipLevels = 1;
        luminanceImageCreateInfo.arrayLayers = 1;
        luminanceImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        luminanceImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        luminanceImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment
                                                 | vk::ImageUsageFlagBits::eInputAttachment
                                                 | vk::ImageUsageFlagBits::eStorage;
        luminanceImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        luminanceImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        for (size_t i = 0; i != 2; ++i) {
            auto createImage_result = vma_allocator.createImage(luminanceImageCreateInfo, image_allocation_info).value;
            luminanceImages[i] = createImage_result.first;
            luminanceAllocations[i] = createImage_result.second;

            vk::ImageViewCreateInfo imageView_create_info;
            imageView_create_info.image = luminanceImages[i];
            imageView_create_info.viewType = vk::ImageViewType::e2D;
            imageView_create_info.format = luminanceImageCreateInfo.format;
            imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity};
            imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                      0, 1,
                                                      0, 1};

            luminanceImageViews[i] = device.createImageView(imageView_create_info).value;

            // Initialize
            OneShotCommandBuffer one_shot_command_buffer(device);
            vk::CommandBuffer command_buffer = one_shot_command_buffer.BeginCommandRecord(graphicsQueue);

            vk::ImageMemoryBarrier init_image_barrier;
            init_image_barrier.image = luminanceImages[i];
            init_image_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            init_image_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            init_image_barrier.oldLayout = vk::ImageLayout::eUndefined;
            init_image_barrier.newLayout = vk::ImageLayout::eGeneral;
            init_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            init_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            init_image_barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                   0, 1,
                                                   0, 1};

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAllCommands,
                                           vk::PipelineStageFlagBits::eAllCommands,
                                           vk::DependencyFlagBits::eByRegion,
                                           0, nullptr,
                                           0, nullptr,
                                           1, &init_image_barrier);

            one_shot_command_buffer.EndAndSubmitCommands();
        }
    }
}

void RealtimeRenderer::InitNRD()
{
    assert(NRDmethod == nrd::Method::REBLUR_DIFFUSE_SPECULAR || NRDmethod == nrd::Method::RELAX_DIFFUSE_SPECULAR);

    uint32_t width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
    uint32_t height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;

    // Create NRDintegration
    const nrd::MethodDesc methods[] = {NRDmethod, (uint16_t)width, (uint16_t)height};
    nrd::DenoiserCreationDesc denoiserCreationDesc = {};
    denoiserCreationDesc.requestedMethods = methods;
    denoiserCreationDesc.requestedMethodNum = 1;

    NRDintegration_uptr = std::make_unique<NRDintegration>(device, vma_allocator, graphics_ptr->GetPipelineFactory(), denoiserCreationDesc);

    // Bind textures
    NRDintegration_uptr->BindTexture(nrd::ResourceType::IN_MV, motionImage, motionImageCreateInfo,
                                     vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);
    NRDintegration_uptr->BindTexture(nrd::ResourceType::IN_NORMAL_ROUGHNESS, normalRoughnessImage, normalRoughnessImageCreateInfo,
                                     vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);
    NRDintegration_uptr->BindTexture(nrd::ResourceType::IN_VIEWZ, linearViewZImage, linearViewZImageCreateInfo,
                                     vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);

    NRDintegration_uptr->BindTexture(nrd::ResourceType::IN_DIFF_RADIANCE_HITDIST, diffuseDistanceImage, diffuseDistanceImageCreateInfo,
                                     vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);
    NRDintegration_uptr->BindTexture(nrd::ResourceType::IN_SPEC_RADIANCE_HITDIST, specularDistanceImage, specularDistanceImageCreateInfo,
                                     vk::ImageLayout::eGeneral, vk::ImageLayout::eGeneral);

    NRDintegration_uptr->BindTexture(nrd::ResourceType::OUT_DIFF_RADIANCE_HITDIST, denoisedDiffuseDistanceImage, denoisedDiffuseDistanceImageCreateInfo,
                                     vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);
    NRDintegration_uptr->BindTexture(nrd::ResourceType::OUT_SPEC_RADIANCE_HITDIST, denoisedSpecularDistanceImage, denoisedSpecularDistanceImageCreateInfo,
                                     vk::ImageLayout::eUndefined, vk::ImageLayout::eGeneral);

    // Set method settings
    if (NRDmethod == nrd::Method::REBLUR_DIFFUSE_SPECULAR) {
        NRD_reBLURsettings.antilagIntensitySettings.sensitivityToDarkness = 2.5f;
        NRD_reBLURsettings.antilagIntensitySettings.enable = true;
        NRD_reBLURsettings.prePassMode = nrd::PrePassMode::SIMPLE;
        NRD_reBLURsettings.stabilizationStrength = 0.98f;
        NRD_reBLURsettings.enableAntiFirefly = true;
        NRD_reBLURsettings.blurRadius = 25.f;
        NRD_reBLURsettings.minConvergedStateBaseRadiusScale = 0.15f;
        NRD_reBLURsettings.maxAccumulatedFrameNum = 41;
        NRDintegration_uptr->SetMethodSettings(nrd::Method::REBLUR_DIFFUSE_SPECULAR, &NRD_reBLURsettings);
    } else {
        NRD_reLAXsettings.diffusePrepassBlurRadius = 5.f;
        NRD_reLAXsettings.specularPrepassBlurRadius = 20.f;
        NRD_reLAXsettings.diffuseMaxAccumulatedFrameNum = 41;
        NRD_reLAXsettings.specularMaxAccumulatedFrameNum = 41;
        NRD_reLAXsettings.diffuseHistoryRejectionNormalThreshold = 0.01f;
        NRD_reLAXsettings.specularPhiLuminance = 0.7f;
        NRD_reLAXsettings.diffusePhiLuminance = 1.f;
        NRD_reLAXsettings.specularLobeAngleSlack = 0.35f;
        NRD_reLAXsettings.enableAntiFirefly = true;
        NRDintegration_uptr->SetMethodSettings(nrd::Method::RELAX_DIFFUSE_SPECULAR, &NRD_reLAXsettings);
    }

    // Set const common settings
    NRD_commonSettings.cameraJitter[0] = 0.f;
    NRD_commonSettings.cameraJitter[1] = 0.f;
    NRD_commonSettings.isMotionVectorInWorldSpace = true;
}


void RealtimeRenderer::InitExposure()
{
    std::tuple<vk::Image, vk::ImageView, vk::ImageCreateInfo> images_tuples[2] = {std::make_tuple(luminanceImages[0], luminanceImageViews[0], luminanceImageCreateInfo),
                                                                                  std::make_tuple(luminanceImages[1], luminanceImageViews[1], luminanceImageCreateInfo)};
    exposure_uptr = std::make_unique<Exposure>(device, vma_allocator, graphics_ptr, images_tuples, exposureComputeQueue, false, true);
}


void RealtimeRenderer::InitTLAS()
{
    TLASbuilder_uptr = std::make_unique<TLASbuilder>(device,
                                                     vma_allocator,
                                                     meshComputeQueue.second,
                                                     graphics_ptr->GetMaxInstancesCount());
}

void RealtimeRenderer::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 3);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eInputAttachment,  1);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageImage,  2 * 8 + 3 * 3);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 3 + 1 + 2 + 3,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create host layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Buffer
            vk::DescriptorSetLayoutBinding buffer_binding;
            buffer_binding.binding = 0;
            buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
            buffer_binding.descriptorCount = 1;
            buffer_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(buffer_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        hostDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Create path trace layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Visibility attachment
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 0;
            attach_binding.descriptorType = vk::DescriptorType::eInputAttachment;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(attach_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        pathTraceDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Create resolve layout set
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Diffuse attachment
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 0;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Specular attachments
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 1;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // NormalRoughness attachment
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 2;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Color metalness attachment
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 3;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Light sources
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 4;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        if (useMorphologicalAA) {   // morphological mask
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 5;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Output
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 10;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Luminance output
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 11;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        resolveDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Create morphological AA set layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Input image
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 0;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Morphological mask image
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 1;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Output
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 10;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        morphologicalAAdescriptorSetsLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Allocate host sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(hostDescriptorSetLayout);
        layouts.emplace_back(hostDescriptorSetLayout);
        layouts.emplace_back(hostDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        hostDescriptorSets[0] = descriptor_sets[0];
        hostDescriptorSets[1] = descriptor_sets[1];
        hostDescriptorSets[2] = descriptor_sets[2];
    }
    {   // Allocate path trace set
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(pathTraceDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        pathTraceDescriptorSet = descriptor_sets[0];
    }
    {   // Allocate resolve sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(resolveDescriptorSetLayout);
        layouts.emplace_back(resolveDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        resolveDescriptorSets[0] = descriptor_sets[0];
        resolveDescriptorSets[1] = descriptor_sets[1];
    }
    {   // Allocate morphological sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(morphologicalAAdescriptorSetsLayout);
        layouts.emplace_back(morphologicalAAdescriptorSetsLayout);
        layouts.emplace_back(morphologicalAAdescriptorSetsLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        morphologicalAAdescriptorSets[0] = descriptor_sets[0];
        morphologicalAAdescriptorSets[1] = descriptor_sets[1];
        morphologicalAAdescriptorSets[2] = descriptor_sets[2];
    }

    {   // Write descriptors of host sets
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;

        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;
        for (size_t i = 0; i != 3; ++i) {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = primitivesInstanceBuffer;
            descriptor_buffer_info_uptr->offset = i * primitivesInstanceBufferPartSize;
            descriptor_buffer_info_uptr->range = primitivesInstanceBufferPartSize;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = hostDescriptorSets[i];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        device.updateDescriptorSets(writes_descriptor_set, {});
    }
    {   // Write path trace set
        vk::WriteDescriptorSet write_descriptor_set;
        vk::DescriptorImageInfo descriptor_image_info;
        descriptor_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        descriptor_image_info.imageView = visibilityImageView;
        descriptor_image_info.sampler = nullptr;

        write_descriptor_set.dstSet = pathTraceDescriptorSet;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = vk::DescriptorType::eInputAttachment;
        write_descriptor_set.pImageInfo = &descriptor_image_info;

        device.updateDescriptorSets(1, &write_descriptor_set,
                                    0, nullptr);
    }
    {   // Write compute resolve set
        for (size_t i = 0; i != 2; ++i) {
            vk::WriteDescriptorSet write_descriptor_sets[8];
            vk::DescriptorImageInfo descriptor_image_infos[8];

            descriptor_image_infos[0].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[0].imageView = denoisedDiffuseDistanceImageView;
            descriptor_image_infos[0].sampler = nullptr;

            write_descriptor_sets[0].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[0].dstBinding = 0;
            write_descriptor_sets[0].dstArrayElement = 0;
            write_descriptor_sets[0].descriptorCount = 1;
            write_descriptor_sets[0].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[0].pImageInfo = &descriptor_image_infos[0];

            descriptor_image_infos[1].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[1].imageView = denoisedSpecularDistanceImageView;
            descriptor_image_infos[1].sampler = nullptr;

            write_descriptor_sets[1].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[1].dstBinding = 1;
            write_descriptor_sets[1].dstArrayElement = 0;
            write_descriptor_sets[1].descriptorCount = 1;
            write_descriptor_sets[1].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[1].pImageInfo = &descriptor_image_infos[1];

            descriptor_image_infos[2].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[2].imageView = normalRoughnessImageView;
            descriptor_image_infos[2].sampler = nullptr;

            write_descriptor_sets[2].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[2].dstBinding = 2;
            write_descriptor_sets[2].dstArrayElement = 0;
            write_descriptor_sets[2].descriptorCount = 1;
            write_descriptor_sets[2].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[2].pImageInfo = &descriptor_image_infos[2];

            descriptor_image_infos[3].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[3].imageView = colorMetalnessImageView;
            descriptor_image_infos[3].sampler = nullptr;

            write_descriptor_sets[3].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[3].dstBinding = 3;
            write_descriptor_sets[3].dstArrayElement = 0;
            write_descriptor_sets[3].descriptorCount = 1;
            write_descriptor_sets[3].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[3].pImageInfo = &descriptor_image_infos[3];

            descriptor_image_infos[4].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[4].imageView = lightSourcesPassImageView;
            descriptor_image_infos[4].sampler = nullptr;

            write_descriptor_sets[4].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[4].dstBinding = 4;
            write_descriptor_sets[4].dstArrayElement = 0;
            write_descriptor_sets[4].descriptorCount = 1;
            write_descriptor_sets[4].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[4].pImageInfo = &descriptor_image_infos[4];

            descriptor_image_infos[5].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[5].imageView = resolveResultImageView;
            descriptor_image_infos[5].sampler = nullptr;

            write_descriptor_sets[5].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[5].dstBinding = 10;
            write_descriptor_sets[5].dstArrayElement = 0;
            write_descriptor_sets[5].descriptorCount = 1;
            write_descriptor_sets[5].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[5].pImageInfo = &descriptor_image_infos[5];

            descriptor_image_infos[6].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[6].imageView = luminanceImageViews[i];
            descriptor_image_infos[6].sampler = nullptr;

            write_descriptor_sets[6].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[6].dstBinding = 11;
            write_descriptor_sets[6].dstArrayElement = 0;
            write_descriptor_sets[6].descriptorCount = 1;
            write_descriptor_sets[6].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[6].pImageInfo = &descriptor_image_infos[6];

            // In case morphological AA is used
            descriptor_image_infos[7].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[7].imageView = morphologicalMaskImageView;
            descriptor_image_infos[7].sampler = nullptr;

            write_descriptor_sets[7].dstSet = resolveDescriptorSets[i];
            write_descriptor_sets[7].dstBinding = 5;
            write_descriptor_sets[7].dstArrayElement = 0;
            write_descriptor_sets[7].descriptorCount = 1;
            write_descriptor_sets[7].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[7].pImageInfo = &descriptor_image_infos[7];

            device.updateDescriptorSets(useMorphologicalAA ? 8 : 7, write_descriptor_sets,
                                        0, nullptr);
        }
    }
    {   // Write morphological AA sets
        for (size_t i = 0; i != 3; ++i) {
            vk::WriteDescriptorSet write_descriptor_sets[2];
            vk::DescriptorImageInfo descriptor_image_infos[2];

            descriptor_image_infos[0].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[0].imageView = resolveResultImageView;
            descriptor_image_infos[0].sampler = nullptr;

            write_descriptor_sets[0].dstSet = morphologicalAAdescriptorSets[i];
            write_descriptor_sets[0].dstBinding = 0;
            write_descriptor_sets[0].dstArrayElement = 0;
            write_descriptor_sets[0].descriptorCount = 1;
            write_descriptor_sets[0].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[0].pImageInfo = &descriptor_image_infos[0];

            descriptor_image_infos[1].imageLayout = vk::ImageLayout::eGeneral;
            descriptor_image_infos[1].imageView = morphologicalMaskImageView;
            descriptor_image_infos[1].sampler = nullptr;

            write_descriptor_sets[1].dstSet = morphologicalAAdescriptorSets[i];
            write_descriptor_sets[1].dstBinding = 1;
            write_descriptor_sets[1].dstArrayElement = 0;
            write_descriptor_sets[1].descriptorCount = 1;
            write_descriptor_sets[1].descriptorType = vk::DescriptorType::eStorageImage;
            write_descriptor_sets[1].pImageInfo = &descriptor_image_infos[1];

            device.updateDescriptorSets(2, write_descriptor_sets,
                                        0, nullptr);
        }
    }
}

void RealtimeRenderer::InitRenderpasses()
{
    std::vector<vk::AttachmentDescription2> attachment_descriptions;

    {   // 0
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = visibilityImageCreateInfo.format;
        attachment_description.samples = useMorphologicalAA ? MAAsamplesCount : vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eClear;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 1
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = depthImageCreateInfo.format;
        attachment_description.samples = useMorphologicalAA ? MAAsamplesCount : vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eClear;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 2
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = diffuseDistanceImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 3
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = specularDistanceImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 4
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = normalRoughnessImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 5
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = colorMetalnessImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 6
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = motionImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 7
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = linearViewZImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    {   // 8
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = lightSourcesPassImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eClear;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eClear;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }
    if (useMorphologicalAA) {   // 9
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = depthResolvedImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eDontCare;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
        attachment_descriptions.emplace_back(attachment_description);
    }
    if (useMorphologicalAA) {   // 10
        vk::AttachmentDescription2 attachment_description;
        attachment_description.format = morphologicalMaskImageCreateInfo.format;
        attachment_description.samples = vk::SampleCountFlagBits::e1;
        attachment_description.loadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.storeOp = vk::AttachmentStoreOp::eStore;
        attachment_description.stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
        attachment_description.stencilStoreOp = vk::AttachmentStoreOp::eStore;
        attachment_description.initialLayout = vk::ImageLayout::eUndefined;
        attachment_description.finalLayout = vk::ImageLayout::eGeneral;
        attachment_descriptions.emplace_back(attachment_description);
    }


    // Subpasses
    std::vector<vk::SubpassDescription2> subpasses;
    std::vector<vk::SubpassDependency2> dependencies;

    // ------------------------------------------------------------------------------
    // Subpass 0 (visibility)
    std::vector<vk::AttachmentReference2> visibility_colorAttachments_refs;
    {
        vk::AttachmentReference2 visibilityAttach_ref;
        visibilityAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        visibilityAttach_ref.attachment = 0;
        visibility_colorAttachments_refs.emplace_back(visibilityAttach_ref);
    }

    vk::AttachmentReference2 visibility_depthAttach_ref;
    visibility_depthAttach_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    visibility_depthAttach_ref.attachment = 1;

    vk::SubpassDescription2 visibility_subpass_description;
    visibility_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    visibility_subpass_description.setColorAttachments(visibility_colorAttachments_refs);
    visibility_subpass_description.pDepthStencilAttachment = &visibility_depthAttach_ref;

    // if useMorphologicalAA
    vk::AttachmentReference2 depthResolved_ref;
    depthResolved_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthResolved_ref.attachment = 9;

    vk::SubpassDescriptionDepthStencilResolve depthResolve_description;
    depthResolve_description.depthResolveMode = vk::ResolveModeFlagBits::eMin;
    depthResolve_description.stencilResolveMode = vk::ResolveModeFlagBits::eNone;
    depthResolve_description.pDepthStencilResolveAttachment = &depthResolved_ref;
    if (useMorphologicalAA) {
        visibility_subpass_description.pNext = &depthResolve_description;
    }

    vk::SubpassDependency2 visibility_subpass_dependency;
    visibility_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    visibility_subpass_dependency.dstSubpass = 0;
    visibility_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    visibility_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    visibility_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    visibility_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
    visibility_subpass_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpasses.emplace_back(visibility_subpass_description);
    dependencies.emplace_back(visibility_subpass_dependency);

    // ------------------------------------------------------------------------------
    // Subpass 1 (path trace)
    std::vector<vk::AttachmentReference2> pathTrace_colorAttachments_refs;
    {
        vk::AttachmentReference2 diffuseDistanceAttach_ref;
        diffuseDistanceAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        diffuseDistanceAttach_ref.attachment = 2;
        pathTrace_colorAttachments_refs.emplace_back(diffuseDistanceAttach_ref);

        vk::AttachmentReference2 specularDistanceAttach_ref;
        specularDistanceAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        specularDistanceAttach_ref.attachment = 3;
        pathTrace_colorAttachments_refs.emplace_back(specularDistanceAttach_ref);

        vk::AttachmentReference2 normalRoughnessAttach_ref;
        normalRoughnessAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        normalRoughnessAttach_ref.attachment = 4;
        pathTrace_colorAttachments_refs.emplace_back(normalRoughnessAttach_ref);

        vk::AttachmentReference2 colorMetalnessAttach_ref;
        colorMetalnessAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        colorMetalnessAttach_ref.attachment = 5;
        pathTrace_colorAttachments_refs.emplace_back(colorMetalnessAttach_ref);

        vk::AttachmentReference2 motionAttach_ref;
        motionAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        motionAttach_ref.attachment = 6;
        pathTrace_colorAttachments_refs.emplace_back(motionAttach_ref);

        vk::AttachmentReference2 viewZAttach_ref;
        viewZAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        viewZAttach_ref.attachment = 7;
        pathTrace_colorAttachments_refs.emplace_back(viewZAttach_ref);
    }
    if (useMorphologicalAA) {
        vk::AttachmentReference2 morphMask_ref;
        morphMask_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        morphMask_ref.attachment = 10;
        pathTrace_colorAttachments_refs.emplace_back(morphMask_ref);
    }

    std::vector<vk::AttachmentReference2> pathTrace_inputAttachments_refs;
    {
        vk::AttachmentReference2 inputAttach_ref;
        inputAttach_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        inputAttach_ref.aspectMask = vk::ImageAspectFlagBits::eColor;
        inputAttach_ref.attachment = 0;
        pathTrace_inputAttachments_refs.emplace_back(inputAttach_ref);
    }

    vk::SubpassDescription2 pathTrace_subpass_description;
    pathTrace_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    pathTrace_subpass_description.setColorAttachments(pathTrace_colorAttachments_refs);
    pathTrace_subpass_description.setInputAttachments(pathTrace_inputAttachments_refs);

    vk::SubpassDependency2 pathTrace_subpass_dependency;
    pathTrace_subpass_dependency.srcSubpass = 0;
    pathTrace_subpass_dependency.dstSubpass = 1;
    pathTrace_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    pathTrace_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    pathTrace_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    pathTrace_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;
    pathTrace_subpass_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpasses.emplace_back(pathTrace_subpass_description);
    dependencies.emplace_back(pathTrace_subpass_dependency);

    // ------------------------------------------------------------------------------
    // Subpass 2 (draw light sources)
    std::vector<vk::AttachmentReference2> lightDraw_colorAttachments_refs;
    {
        vk::AttachmentReference2 lightDrawAttach_ref;
        lightDrawAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        lightDrawAttach_ref.attachment = 8;
        lightDraw_colorAttachments_refs.emplace_back(lightDrawAttach_ref);
    }

    vk::AttachmentReference2 lightDraw_depthAttach_ref;
    lightDraw_depthAttach_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    lightDraw_depthAttach_ref.attachment = useMorphologicalAA? 9 : 1;

    vk::SubpassDescription2 lightDraw_subpass_description;
    lightDraw_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    lightDraw_subpass_description.setColorAttachments(lightDraw_colorAttachments_refs);
    lightDraw_subpass_description.pDepthStencilAttachment = &lightDraw_depthAttach_ref;

    vk::SubpassDependency2 lightDraw_subpass_dependency;
    lightDraw_subpass_dependency.srcSubpass = 0;
    lightDraw_subpass_dependency.dstSubpass = 2;
    lightDraw_subpass_dependency.srcStageMask = useMorphologicalAA? vk::PipelineStageFlagBits::eColorAttachmentOutput
                                                                  : vk::PipelineStageFlagBits::eLateFragmentTests;
    lightDraw_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    lightDraw_subpass_dependency.srcAccessMask = useMorphologicalAA? vk::AccessFlagBits::eColorAttachmentWrite
                                                                   : vk::AccessFlagBits::eDepthStencilAttachmentWrite;
    lightDraw_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentRead;
    lightDraw_subpass_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpasses.emplace_back(lightDraw_subpass_description);
    dependencies.emplace_back(lightDraw_subpass_dependency);

    // ------------------------------------------------------------------------------
    // Renderpass
    vk::RenderPassCreateInfo2 renderpass_create_info;
    renderpass_create_info.setAttachments(attachment_descriptions);
    renderpass_create_info.setSubpasses(subpasses);
    renderpass_create_info.setDependencies(dependencies);

    renderpass = device.createRenderPass2(renderpass_create_info).value;
}

void RealtimeRenderer::InitFramebuffers()
{
    std::vector<vk::ImageView> attachments;
    attachments.emplace_back(visibilityImageView);
    attachments.emplace_back(depthImageView);
    attachments.emplace_back(diffuseDistanceImageView);
    attachments.emplace_back(specularDistanceImageView);
    attachments.emplace_back(normalRoughnessImageView);
    attachments.emplace_back(colorMetalnessImageView);
    attachments.emplace_back(motionImageView);
    attachments.emplace_back(linearViewZImageView);
    attachments.emplace_back(lightSourcesPassImageView);
    if (useMorphologicalAA) {
        attachments.emplace_back(depthResolvedImageView);
        attachments.emplace_back(morphologicalMaskImageView);
    }

    vk::FramebufferCreateInfo framebuffer_createInfo;
    framebuffer_createInfo.renderPass = renderpass;
    framebuffer_createInfo.setAttachments(attachments);
    framebuffer_createInfo.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
    framebuffer_createInfo.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
    framebuffer_createInfo.layers = 1;

    frameBuffer = device.createFramebuffer(framebuffer_createInfo).value;
}

void RealtimeRenderer::InitSemaphoresAndFences()
{
    {   // readyForPresentSemaphores
        vk::SemaphoreCreateInfo semaphore_create_info;

        readyForPresentSemaphores[0] = device.createSemaphore(semaphore_create_info).value;
        readyForPresentSemaphores[1] = device.createSemaphore(semaphore_create_info).value;
        readyForPresentSemaphores[2] = device.createSemaphore(semaphore_create_info).value;
    }

    {   // presentImageAvailableSemaphores
        vk::SemaphoreCreateInfo semaphore_create_info;

        presentImageAvailableSemaphores[0] = device.createSemaphore(semaphore_create_info).value;
        presentImageAvailableSemaphores[1] = device.createSemaphore(semaphore_create_info).value;
        presentImageAvailableSemaphores[2] = device.createSemaphore(semaphore_create_info).value;
    }

    {   // transformsFinishTimelineSemaphore
        vk::SemaphoreTypeCreateInfo semaphore_type_info;
        semaphore_type_info.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphore_type_info.initialValue = 0;

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_info;

        transformsFinishTimelineSemaphore = device.createSemaphore(semaphore_create_info).value;
    }

    {   // xLASupdateFinishTimelineSemaphore
        vk::SemaphoreTypeCreateInfo semaphore_type_info;
        semaphore_type_info.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphore_type_info.initialValue = 0;

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_info;

        xLASupdateFinishTimelineSemaphore = device.createSemaphore(semaphore_create_info).value;
    }

    {   // graphicsFinishTimelineSemaphore
        vk::SemaphoreTypeCreateInfo semaphore_type_info;
        semaphore_type_info.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphore_type_info.initialValue = 0;

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_info;

        graphicsFinishTimelineSemaphore = device.createSemaphore(semaphore_create_info).value;
    }

    {   // histogramFinishTimelineSemaphore
        vk::SemaphoreTypeCreateInfo semaphore_type_info;
        semaphore_type_info.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphore_type_info.initialValue = 0;

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_info;

        histogramFinishTimelineSemaphore = device.createSemaphore(semaphore_create_info).value;
    }
}

void RealtimeRenderer::InitCommandBuffers()
{
    {   // graphics command buffers
        vk::CommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool_create_info.queueFamilyIndex = graphicsQueue.second;

        graphicsCommandPool = device.createCommandPool(command_pool_create_info).value;

        vk::CommandBufferAllocateInfo command_buffer_alloc_info;
        command_buffer_alloc_info.commandPool = graphicsCommandPool;
        command_buffer_alloc_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_alloc_info.commandBufferCount = 3;

        auto command_buffers = device.allocateCommandBuffers(command_buffer_alloc_info).value;
        graphicsCommandBuffers[0] = command_buffers[0];
        graphicsCommandBuffers[1] = command_buffers[1];
        graphicsCommandBuffers[2] = command_buffers[2];
    }
    {   // compute command buffers
        vk::CommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
        command_pool_create_info.queueFamilyIndex = meshComputeQueue.second;

        computeCommandPool = device.createCommandPool(command_pool_create_info).value;

        vk::CommandBufferAllocateInfo command_buffer_alloc_info;
        command_buffer_alloc_info.commandPool = computeCommandPool;
        command_buffer_alloc_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_alloc_info.commandBufferCount = 9;

        auto command_buffers = device.allocateCommandBuffers(command_buffer_alloc_info).value;
        transformCommandBuffers[0] = command_buffers[0];
        transformCommandBuffers[1] = command_buffers[1];
        transformCommandBuffers[2] = command_buffers[2];
        xLASCommandBuffers[0] = command_buffers[3];
        xLASCommandBuffers[1] = command_buffers[4];
        xLASCommandBuffers[2] = command_buffers[5];
        exposureCommandBuffers[0] = command_buffers[6];
        exposureCommandBuffers[1] = command_buffers[7];
        exposureCommandBuffers[2] = command_buffers[8];
    }
}

void RealtimeRenderer::InitPrimitivesSet()
{
    // Create primitives sets (shaders-pipelines for each kind of primitive)
    printf("-Initializing \"Visibility Pass\" primitives set\n");
    for(size_t i = 0; i != graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitivesCount(); ++i)
    {
        const PrimitiveInfo& this_primitiveInfo = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(i);
        const MaterialAbout& this_material = graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialAbout(this_primitiveInfo.material);

        // No transparent
        if (this_material.transparent) {
            primitivesPipelineLayouts.emplace_back(nullptr);
            primitivesPipelines.emplace_back(nullptr);
            continue;
        }

        std::vector<std::pair<std::string, std::string>> shadersDefinitionStringPairs = this_material.definitionStringPairs;
        shadersDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));

        // Pipeline layout
        vk::PipelineLayout this_pipeline_layout;
        {
            vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

            std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
            descriptor_sets_layouts.emplace_back(graphics_ptr->GetCameraDescriptionSetLayout());
            descriptor_sets_layouts.emplace_back(graphics_ptr->GetMatricesDescriptionSetLayout());
            if (this_material.masked)
                descriptor_sets_layouts.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSetLayout());
            pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

            std::vector<vk::PushConstantRange> push_constant_range;
            push_constant_range.emplace_back(vk::ShaderStageFlagBits::eVertex, 0, 4);
            if (not this_material.masked)
                push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 4, 4);
            else
                push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 4 ,8);

            pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

            this_pipeline_layout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
        }

        // Pipeline
        vk::Pipeline this_pipeline;
        {
            vk::GraphicsPipelineCreateInfo pipeline_create_info;

            // PipelineVertexInputStateCreateInfo
            vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
            std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions;
            std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;

            uint32_t binding_index = 0;
            uint32_t location_index = 0;

            vertex_input_binding_descriptions.emplace_back(binding_index,
                                                           uint32_t(4 * sizeof(float)),
                                                           vk::VertexInputRate::eVertex);
            vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                             vk::Format::eR32G32B32A32Sfloat,
                                                             0);
            ++binding_index; ++location_index;

            if (this_material.masked) {
                vertex_input_binding_descriptions.emplace_back(binding_index,
                                                               uint32_t(2 * sizeof(float)) * this_primitiveInfo.texcoordsCount,
                                                               vk::VertexInputRate::eVertex);
                vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                                 vk::Format::eR32G32Sfloat,
                                                                 0);
                ++binding_index; ++location_index;
            }

            vertex_input_state_create_info.setVertexBindingDescriptions(vertex_input_binding_descriptions);
            vertex_input_state_create_info.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);

            pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;

            // PipelineInputAssemblyStateCreateInfo
            vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
            input_assembly_state_create_info.topology = this_primitiveInfo.drawMode;

            pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;

            // Tesselation
            pipeline_create_info.pTessellationState = nullptr;

            // PipelineViewportStateCreateInfo
            vk::PipelineViewportStateCreateInfo viewport_state_create_info;

            uint32_t width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
            uint32_t height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;

            vk::Viewport viewport {0.f, 0.f,
                                   float(width), float(height),
                                   0.f, 1.f};
            viewport_state_create_info.viewportCount = 1;
            viewport_state_create_info.pViewports = &viewport;

            vk::Rect2D scissor {{0, 0}, {width, height}};
            viewport_state_create_info.scissorCount = 1;
            viewport_state_create_info.pScissors = &scissor;

            pipeline_create_info.pViewportState = &viewport_state_create_info;

            // PipelineRasterizationStateCreateInfo
            vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
            rasterization_state_create_info.depthBiasClamp = VK_FALSE;
            rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
            rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
            rasterization_state_create_info.cullMode = this_material.twoSided ? vk::CullModeFlagBits::eNone : vk::CullModeFlagBits::eBack;
            rasterization_state_create_info.frontFace = vk::FrontFace::eCounterClockwise;
            rasterization_state_create_info.lineWidth = 1.f;

            pipeline_create_info.pRasterizationState = &rasterization_state_create_info;

            // PipelineMultisampleStateCreateInfo
            vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
            multisample_state_create_info.rasterizationSamples = useMorphologicalAA ? MAAsamplesCount : vk::SampleCountFlagBits::e1;
            multisample_state_create_info.sampleShadingEnable = VK_FALSE;
            multisample_state_create_info.minSampleShading = 1.0f;
            multisample_state_create_info.pSampleMask = nullptr;
            multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
            multisample_state_create_info.alphaToOneEnable = VK_FALSE;

            pipeline_create_info.pMultisampleState = &multisample_state_create_info;

            // PipelineDepthStencilStateCreateInfo
            vk::PipelineDepthStencilStateCreateInfo depth_state_create_info;
            depth_state_create_info.depthTestEnable = VK_TRUE;
            depth_state_create_info.depthWriteEnable = VK_TRUE;
            depth_state_create_info.depthCompareOp = vk::CompareOp::eLess;
            depth_state_create_info.depthBoundsTestEnable = VK_FALSE;
            depth_state_create_info.stencilTestEnable = VK_FALSE;
            depth_state_create_info.minDepthBounds = 0.f;
            depth_state_create_info.maxDepthBounds = 1.f;

            pipeline_create_info.pDepthStencilState = &depth_state_create_info;

            // PipelineColorBlendAttachmentState
            vk::PipelineColorBlendStateCreateInfo color_blend_create_info;
            vk::PipelineColorBlendAttachmentState color_blend_attachment;
            color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
            color_blend_attachment.blendEnable = VK_FALSE;
            color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
            color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
            color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
            color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
            color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
            color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;

            color_blend_create_info.logicOpEnable = VK_FALSE;
            color_blend_create_info.attachmentCount = 1;
            color_blend_create_info.pAttachments = &color_blend_attachment;

            pipeline_create_info.pColorBlendState = &color_blend_create_info;

            // PipelineShaderStageCreateInfo
            std::vector<vk::PipelineShaderStageCreateInfo> shaders_stage_create_infos;

            ShadersSpecs shaders_specs {"Realtime Renderer - Visibility Shaders", shadersDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            assert(shader_set.abortedDueToDefinition == false);

            vk::PipelineShaderStageCreateInfo vertex_shader_stage;
            vertex_shader_stage.stage = vk::ShaderStageFlagBits::eVertex;
            vertex_shader_stage.module = shader_set.vertexShaderModule;
            vertex_shader_stage.pName = "main";
            shaders_stage_create_infos.emplace_back(vertex_shader_stage);

            vk::PipelineShaderStageCreateInfo fragment_shader_stage;
            fragment_shader_stage.stage = vk::ShaderStageFlagBits::eFragment;
            fragment_shader_stage.module = shader_set.fragmentShaderModule;
            fragment_shader_stage.pName = "main";
            shaders_stage_create_infos.emplace_back(fragment_shader_stage);

            pipeline_create_info.setStages(shaders_stage_create_infos);

            // etc
            pipeline_create_info.layout = this_pipeline_layout;
            pipeline_create_info.renderPass = renderpass;
            pipeline_create_info.subpass = 0;

            this_pipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(pipeline_create_info).first;
        }

        primitivesPipelineLayouts.emplace_back(this_pipeline_layout);
        primitivesPipelines.emplace_back(this_pipeline);
    }
}

void RealtimeRenderer::InitPathTracePipeline()
{
    printf("-Initializing \"Path-Trace Pass\" pipeline\n");

    std::vector<std::pair<std::string, std::string>> shadersDefinitionStringPairs;
    shadersDefinitionStringPairs.emplace_back("TEXTURES_COUNT", std::to_string(graphics_ptr->GetTexturesOfMaterials()->GetTexturesCount()));
    shadersDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
    shadersDefinitionStringPairs.emplace_back("INSTANCES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
    shadersDefinitionStringPairs.emplace_back("MATERIALS_PARAMETERS_COUNT", std::to_string(graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialsCount()));
    shadersDefinitionStringPairs.emplace_back("MAX_LIGHTS_COUNT", std::to_string(graphics_ptr->GetLights()->GetMaxLights()));
    shadersDefinitionStringPairs.emplace_back("MAX_COMBINATIONS_SIZE", std::to_string(graphics_ptr->GetLights()->GetLightsCombinationsSize()));
    shadersDefinitionStringPairs.emplace_back("FP16_FACTOR", std::to_string(FP16factor));
    if (useMorphologicalAA)
        shadersDefinitionStringPairs.emplace_back("MORPHOLOGICAL_MSAA", vk::to_string(MAAsamplesCount));
    if (NRDmethod == nrd::Method::REBLUR_DIFFUSE_SPECULAR)
        shadersDefinitionStringPairs.emplace_back("DENOISER_REBLUR", "");

    {   // Pipeline layout fullscreen
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetCameraDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMatricesDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetDynamicMeshes()->GetDescriptorLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetDynamicMeshes()->GetDescriptorLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSetLayout());
        descriptor_sets_layouts.emplace_back(hostDescriptorSetLayout);
        descriptor_sets_layouts.emplace_back(pathTraceDescriptorSetLayout);
        descriptor_sets_layouts.emplace_back(TLASbuilder_uptr->GetDescriptorSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetLights()->GetDescriptorSetLayout());
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 0, 1 * sizeof(glm::vec4) + 1 * sizeof(glm::uvec2) + 3 * sizeof(uint32_t) + sizeof(float));

        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        pathTracePipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
    }

    {   // Pipeline layout
        vk::GraphicsPipelineCreateInfo pipeline_create_info;

        // PipelineVertexInputStateCreateInfo
        vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
        std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;

        uint32_t binding_index = 0;
        uint32_t location_index = 0;

        vertex_input_binding_descriptions.emplace_back(binding_index,
                                                       uint32_t(sizeof(glm::vec4)),
                                                       vk::VertexInputRate::eVertex);
        vertex_input_attribute_descriptions.emplace_back(location_index++, binding_index,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         0);
        vertex_input_attribute_descriptions.emplace_back(location_index++, binding_index,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         uint32_t(3 * sizeof(glm::vec4)));

        ++binding_index;

        vertex_input_state_create_info.setVertexBindingDescriptions(vertex_input_binding_descriptions);
        vertex_input_state_create_info.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);

        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;

        // PipelineInputAssemblyStateCreateInfo
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
        input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleFan;

        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;

        // PipelineViewportStateCreateInfo
        vk::PipelineViewportStateCreateInfo viewport_state_create_info;

        uint32_t width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        uint32_t height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;

        vk::Viewport viewport {0.f, 0.f,
                               float(width), float(height),
                               0.f, 1.f};
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;

        vk::Rect2D scissor {{0, 0}, {width, height}};
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissor;

        pipeline_create_info.pViewportState = &viewport_state_create_info;

        // PipelineRasterizationStateCreateInfo
        vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
        rasterization_state_create_info.depthBiasClamp = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
        rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eNone;
        rasterization_state_create_info.frontFace = vk::FrontFace::eCounterClockwise;
        rasterization_state_create_info.lineWidth = 1.f;

        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;

        // PipelineMultisampleStateCreateInfo
        vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
        multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        multisample_state_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisample_state_create_info.minSampleShading = 1.0f;
        multisample_state_create_info.pSampleMask = nullptr;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        pipeline_create_info.pMultisampleState = &multisample_state_create_info;

        // PipelineDepthStencilStateCreateInfo
        vk::PipelineDepthStencilStateCreateInfo depth_state_create_info;
        depth_state_create_info.depthTestEnable = VK_FALSE;
        depth_state_create_info.depthWriteEnable = VK_FALSE;
        depth_state_create_info.depthCompareOp = vk::CompareOp::eLess;
        depth_state_create_info.depthBoundsTestEnable = VK_FALSE;
        depth_state_create_info.stencilTestEnable = VK_FALSE;
        depth_state_create_info.minDepthBounds = 0.f;
        depth_state_create_info.maxDepthBounds = 1.f;

        pipeline_create_info.pDepthStencilState = &depth_state_create_info;

        // PipelineColorBlendAttachmentState
        vk::PipelineColorBlendStateCreateInfo color_blend_create_info;
        vk::PipelineColorBlendAttachmentState color_blend_attachments[7];
        color_blend_attachments[0].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[0].blendEnable = VK_FALSE;
        color_blend_attachments[1].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[1].blendEnable = VK_FALSE;
        color_blend_attachments[2].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[2].blendEnable = VK_FALSE;
        color_blend_attachments[3].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[3].blendEnable = VK_FALSE;
        color_blend_attachments[4].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[4].blendEnable = VK_FALSE;
        color_blend_attachments[5].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[5].blendEnable = VK_FALSE;
        color_blend_attachments[6].colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                    vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachments[6].blendEnable = VK_FALSE;

        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.attachmentCount = useMorphologicalAA ? 7 : 6;
        color_blend_create_info.pAttachments = color_blend_attachments;

        pipeline_create_info.pColorBlendState = &color_blend_create_info;

        // PipelineShaderStageCreateInfo
        std::vector<vk::PipelineShaderStageCreateInfo> shaders_stage_create_infos;

        ShadersSpecs shaders_specs {"Realtime Renderer - Path-Trace Shaders", shadersDefinitionStringPairs};
        ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

        assert(shader_set.abortedDueToDefinition == false);

        vk::PipelineShaderStageCreateInfo vertex_shader_stage;
        vertex_shader_stage.stage = vk::ShaderStageFlagBits::eVertex;
        vertex_shader_stage.module = shader_set.vertexShaderModule;
        vertex_shader_stage.pName = "main";
        shaders_stage_create_infos.emplace_back(vertex_shader_stage);

        vk::PipelineShaderStageCreateInfo fragment_shader_stage;
        fragment_shader_stage.stage = vk::ShaderStageFlagBits::eFragment;
        fragment_shader_stage.module = shader_set.fragmentShaderModule;
        fragment_shader_stage.pName = "main";
        shaders_stage_create_infos.emplace_back(fragment_shader_stage);

        pipeline_create_info.setStages(shaders_stage_create_infos);

        // etc
        pipeline_create_info.layout = pathTracePipelineLayout;
        pipeline_create_info.renderPass = renderpass;
        pipeline_create_info.subpass = 1;

        pathTracePipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(pipeline_create_info).first;
    }
}

void RealtimeRenderer::InitLightsDrawPipeline()
{
    printf("-Initializing \"Lights-draw Pass\" pipeline\n");

    std::vector<std::pair<std::string, std::string>> shadersDefinitionStringPairs;
    shadersDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
    shadersDefinitionStringPairs.emplace_back("INSTANCES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));

    // Pipeline layout
    {
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetCameraDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMatricesDescriptionSetLayout());
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eVertex, 0, 12);
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 16, 16);
        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        lightDrawPipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
    }

    // Pipeline
    {
        vk::GraphicsPipelineCreateInfo pipeline_create_info;

        // PipelineVertexInputStateCreateInfo
        vk::PipelineVertexInputStateCreateInfo vertex_input_state_create_info;
        std::vector<vk::VertexInputBindingDescription> vertex_input_binding_descriptions;
        std::vector<vk::VertexInputAttributeDescription> vertex_input_attribute_descriptions;

        uint32_t binding_index = 0;
        uint32_t location_index = 0;

        vertex_input_binding_descriptions.emplace_back(binding_index,
                                                       uint32_t(4 * sizeof(float)),
                                                       vk::VertexInputRate::eVertex);
        vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         0);
        ++binding_index;
        ++location_index;

        vertex_input_state_create_info.setVertexBindingDescriptions(vertex_input_binding_descriptions);
        vertex_input_state_create_info.setVertexAttributeDescriptions(vertex_input_attribute_descriptions);

        pipeline_create_info.pVertexInputState = &vertex_input_state_create_info;

        // PipelineInputAssemblyStateCreateInfo
        vk::PipelineInputAssemblyStateCreateInfo input_assembly_state_create_info;
        input_assembly_state_create_info.topology = vk::PrimitiveTopology::eTriangleList;

        pipeline_create_info.pInputAssemblyState = &input_assembly_state_create_info;

        // Tesselation
        pipeline_create_info.pTessellationState = nullptr;

        // PipelineViewportStateCreateInfo
        vk::PipelineViewportStateCreateInfo viewport_state_create_info;

        uint32_t width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        uint32_t height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;

        vk::Viewport viewport{0.f, 0.f,
                              float(width), float(height),
                              0.f, 1.f};
        viewport_state_create_info.viewportCount = 1;
        viewport_state_create_info.pViewports = &viewport;

        vk::Rect2D scissor{{0,0},
                           {width, height}};
        viewport_state_create_info.scissorCount = 1;
        viewport_state_create_info.pScissors = &scissor;

        pipeline_create_info.pViewportState = &viewport_state_create_info;

        // PipelineRasterizationStateCreateInfo
        vk::PipelineRasterizationStateCreateInfo rasterization_state_create_info;
        rasterization_state_create_info.depthBiasClamp = VK_FALSE;
        rasterization_state_create_info.rasterizerDiscardEnable = VK_FALSE;
        rasterization_state_create_info.polygonMode = vk::PolygonMode::eFill;
        rasterization_state_create_info.cullMode = vk::CullModeFlagBits::eBack;
        rasterization_state_create_info.frontFace = vk::FrontFace::eCounterClockwise;
        rasterization_state_create_info.lineWidth = 1.f;

        pipeline_create_info.pRasterizationState = &rasterization_state_create_info;

        // PipelineMultisampleStateCreateInfo
        vk::PipelineMultisampleStateCreateInfo multisample_state_create_info;
        multisample_state_create_info.sampleShadingEnable = VK_FALSE;
        multisample_state_create_info.rasterizationSamples = vk::SampleCountFlagBits::e1;
        multisample_state_create_info.minSampleShading = 1.0f;
        multisample_state_create_info.pSampleMask = nullptr;
        multisample_state_create_info.alphaToCoverageEnable = VK_FALSE;
        multisample_state_create_info.alphaToOneEnable = VK_FALSE;

        pipeline_create_info.pMultisampleState = &multisample_state_create_info;

        // PipelineDepthStencilStateCreateInfo
        vk::PipelineDepthStencilStateCreateInfo depth_state_create_info;
        depth_state_create_info.depthTestEnable = VK_TRUE;
        depth_state_create_info.depthWriteEnable = VK_FALSE;
        depth_state_create_info.depthCompareOp = vk::CompareOp::eLessOrEqual;
        depth_state_create_info.depthBoundsTestEnable = VK_FALSE;
        depth_state_create_info.stencilTestEnable = VK_FALSE;
        depth_state_create_info.minDepthBounds = 0.f;
        depth_state_create_info.maxDepthBounds = 1.f;

        pipeline_create_info.pDepthStencilState = &depth_state_create_info;

        // PipelineColorBlendAttachmentState
        vk::PipelineColorBlendStateCreateInfo color_blend_create_info;
        vk::PipelineColorBlendAttachmentState color_blend_attachment;
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachment.blendEnable = VK_FALSE;
        color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eZero;
        color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
        color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eZero;
        color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;

        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_attachment;

        pipeline_create_info.pColorBlendState = &color_blend_create_info;

        // PipelineShaderStageCreateInfo
        std::vector<vk::PipelineShaderStageCreateInfo> shaders_stage_create_infos;

        ShadersSpecs shaders_specs{"Realtime Renderer - Light Draw Shaders", shadersDefinitionStringPairs};
        ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

        assert(shader_set.abortedDueToDefinition == false);

        vk::PipelineShaderStageCreateInfo vertex_shader_stage;
        vertex_shader_stage.stage = vk::ShaderStageFlagBits::eVertex;
        vertex_shader_stage.module = shader_set.vertexShaderModule;
        vertex_shader_stage.pName = "main";
        shaders_stage_create_infos.emplace_back(vertex_shader_stage);

        vk::PipelineShaderStageCreateInfo fragment_shader_stage;
        fragment_shader_stage.stage = vk::ShaderStageFlagBits::eFragment;
        fragment_shader_stage.module = shader_set.fragmentShaderModule;
        fragment_shader_stage.pName = "main";
        shaders_stage_create_infos.emplace_back(fragment_shader_stage);

        pipeline_create_info.setStages(shaders_stage_create_infos);

        // etc
        pipeline_create_info.layout = lightDrawPipelineLayout;
        pipeline_create_info.renderPass = renderpass;
        pipeline_create_info.subpass = 2;

        lightDrawPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(pipeline_create_info).first;
    }
}

void RealtimeRenderer::InitResolveComputePipeline()
{
    printf("-Initializing \"Resolve Pass\" pipeline\n");

    { // Create pipeline layout
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetCameraDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(resolveDescriptorSetLayout);
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eCompute, 0, uint32_t(4 * 4 + 3 * 16));
        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        resolveCompPipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
    }

    { // Create pipeline
        std::vector<std::pair<std::string, std::string>> definitionStringPairs;
        definitionStringPairs.emplace_back("FP16_FACTOR", std::to_string(FP16factor));
        definitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(comp_dim_size));
        definitionStringPairs.emplace_back("LOCAL_SIZE_Y", std::to_string(comp_dim_size));
        if (useMorphologicalAA)
            definitionStringPairs.emplace_back("MORPHOLOGICAL_MSAA", vk::to_string(MAAsamplesCount));

        ShadersSpecs shaders_specs = {"Realtime Renderer - Resolve Shader", definitionStringPairs};
        ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

        vk::ComputePipelineCreateInfo compute_pipeline_create_info;
        compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
        compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
        compute_pipeline_create_info.stage.pName = "main";
        compute_pipeline_create_info.layout = resolveCompPipelineLayout;

        resolveCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
    }
}

void RealtimeRenderer::InitMorphologicalAAcomputePipeline()
{
    printf("-Initializing \"Morphological Anti-alias Pass\" pipeline\n");

    { // Create pipeline layout
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(morphologicalAAdescriptorSetsLayout);
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eCompute,0,2 * 4 );
        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        morphologicalAAcompPipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
    }
    { // Create pipeline
        std::vector<std::pair<std::string, std::string>> definitionStringPairs;
        definitionStringPairs.emplace_back("MORPHOLOGICAL_MSAA", vk::to_string(MAAsamplesCount));
        definitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(comp_dim_size));
        definitionStringPairs.emplace_back("LOCAL_SIZE_Y", std::to_string(comp_dim_size));

        ShadersSpecs shaders_specs = {"Realtime Renderer - Morphological AA Shader", definitionStringPairs};
        ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

        vk::ComputePipelineCreateInfo compute_pipeline_create_info;
        compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
        compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
        compute_pipeline_create_info.stage.pName = "main";
        compute_pipeline_create_info.layout = morphologicalAAcompPipelineLayout;

        morphologicalAAcompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
    }
}


void RealtimeRenderer::DrawFrame(const ViewportFrustum &in_viewport,
                                 std::vector<ModelMatrices> &&in_matrices,
                                 std::vector<LightInfo> &&in_light_infos,
                                 std::vector<DrawInfo> &&in_draw_infos)
{


    ++frameCount;
    size_t commandBuffer_index = frameCount % 3;

    if (frameCount == 1) {
        prevFrameViewport = in_viewport;
    } else {
        prevFrameViewport = viewport;
    }

    viewport = in_viewport;
    matrices = in_matrices;
    lightInfos = in_light_infos;
    drawInfos = in_draw_infos;

    //
    // Wait! Wait for command and host buffers (-3)
#ifdef CPU_WAIT_GPU_SERIALIZED
    const uint32_t wait_GPU_frames = 1;
#else
    const uint32_t wait_GPU_frames = 3;
#endif
    if (frameCount > wait_GPU_frames) {
        auto wait_value = uint64_t(frameCount - wait_GPU_frames);

        vk::SemaphoreWaitInfo host_wait_info;
        host_wait_info.semaphoreCount = 1;
        host_wait_info.pSemaphores = &histogramFinishTimelineSemaphore;
        host_wait_info.pValues = &wait_value;

        device.waitSemaphores(host_wait_info, uint64_t(-1));

    }

    std::vector<std::unique_ptr<vk::TimelineSemaphoreSubmitInfo>> timeline_semaphore_infos;
    std::vector<std::unique_ptr<std::vector<vk::Semaphore>>> semaphore_vectors;
    std::vector<std::unique_ptr<std::vector<vk::PipelineStageFlags>>> pipelineStageFlags_vectors;
    std::vector<std::unique_ptr<std::vector<uint64_t>>> semaphore_values_vectors;

    graphics_ptr->GetDynamicMeshes()->PrepareNewFrame(frameCount);
    graphics_ptr->GetLights()->PrepareNewFrame(frameCount);
    exposure_uptr->CalcNextFrameValue(frameCount, graphics_ptr->GetDeltaTimeSeconds());

    graphics_ptr->GetLights()->AddLights(lightInfos, matrices);
    coneLightsIndicesRange = graphics_ptr->GetLights()->CreateLightsConesRange();

    primitive_instance_parameters = CreatePrimitivesInstanceParameters();

    PrepareNRDsettings();
    NRDintegration_uptr->PrepareNewFrame(frameCount, NRD_commonSettings);

    AssortDrawInfos();
    std::vector<DrawInfo> TLAS_draw_infos;
    std::copy(drawStaticMeshInfos.begin(), drawStaticMeshInfos.end(), std::back_inserter(TLAS_draw_infos));
    std::copy(drawDynamicMeshInfos.begin(), drawDynamicMeshInfos.end(), std::back_inserter(TLAS_draw_infos));
    std::copy(drawLocalLightSources.begin(), drawLocalLightSources.end(), std::back_inserter(TLAS_draw_infos));
    TLAS_instances = TLASbuilder::CreateTLASinstances(TLAS_draw_infos, matrices, frameCount%2, graphics_ptr);

    std::vector<vk::SubmitInfo> before_compute_submit_infos;
    {
        vk::CommandBuffer &transform_command_buffer = transformCommandBuffers[commandBuffer_index];
        transform_command_buffer.reset();
        transform_command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        if (frameCount > 3) graphics_ptr->GetDynamicMeshes()->ObtainTransformRanges(transform_command_buffer, drawDynamicMeshInfos, graphicsQueue.second);
        graphics_ptr->GetDynamicMeshes()->RecordTransformations(transform_command_buffer, drawDynamicMeshInfos);

        transform_command_buffer.end();

        vk::SubmitInfo transform_submit_info;
        std::unique_ptr<vk::TimelineSemaphoreSubmitInfo> transform_timeline_semaphore_info = std::make_unique<vk::TimelineSemaphoreSubmitInfo>();
        transform_submit_info.pNext = transform_timeline_semaphore_info.get();
        transform_submit_info.commandBufferCount = 1;
        transform_submit_info.pCommandBuffers = &transform_command_buffer;
        // Transform wait
        auto transform_wait_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto transform_wait_pipeline_stages = std::make_unique<std::vector<vk::PipelineStageFlags>>();
        auto transform_wait_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        if (frameCount > 2) {
            transform_wait_semaphores->emplace_back(graphicsFinishTimelineSemaphore);
            transform_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eTopOfPipe);
            transform_wait_semaphores_values->emplace_back(frameCount - 2);
        }
        transform_submit_info.setWaitSemaphores(*transform_wait_semaphores);
        transform_submit_info.setWaitDstStageMask(*transform_wait_pipeline_stages);
        transform_timeline_semaphore_info->setWaitSemaphoreValues(*transform_wait_semaphores_values);
        // Transform signal
        auto transform_signal_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto transform_signal_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        transform_signal_semaphores->emplace_back(transformsFinishTimelineSemaphore);
        transform_signal_semaphores_values->emplace_back(frameCount);
        transform_submit_info.setSignalSemaphores(*transform_signal_semaphores);
        transform_timeline_semaphore_info->setSignalSemaphoreValues(*transform_signal_semaphores_values);

        // Push-back
        before_compute_submit_infos.emplace_back(transform_submit_info);
        timeline_semaphore_infos.emplace_back(std::move(transform_timeline_semaphore_info));
        semaphore_vectors.emplace_back(std::move(transform_wait_semaphores));
        pipelineStageFlags_vectors.emplace_back(std::move(transform_wait_pipeline_stages));
        semaphore_values_vectors.emplace_back(std::move(transform_wait_semaphores_values));
        semaphore_vectors.emplace_back(std::move(transform_signal_semaphores));
        semaphore_values_vectors.emplace_back(std::move(transform_signal_semaphores_values));
    }
    {
        vk::CommandBuffer &xLAS_command_buffer = xLASCommandBuffers[commandBuffer_index];
        xLAS_command_buffer.reset();
        xLAS_command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        // TODO: Remove cringes
        if (frameCount > 2) graphics_ptr->GetDynamicMeshes()->ObtainBLASranges(xLAS_command_buffer, drawDynamicMeshInfos, graphicsQueue.second);
        graphics_ptr->GetDynamicMeshes()->RecordBLASupdate(xLAS_command_buffer, drawDynamicMeshInfos);
        if (frameCount > 2) TLASbuilder_uptr->ObtainTLASranges(xLAS_command_buffer, frameCount % 2, graphicsQueue.second);
        TLASbuilder_uptr->RecordTLASupdate(xLAS_command_buffer, frameCount % 3, frameCount % 2, TLAS_instances.size());
        graphics_ptr->GetDynamicMeshes()->TransferTransformAndBLASranges(xLAS_command_buffer, drawDynamicMeshInfos, graphicsQueue.second);
        TLASbuilder_uptr->TransferTLASrange(xLAS_command_buffer, frameCount % 2, graphicsQueue.second);

        xLAS_command_buffer.end();

        vk::SubmitInfo xLAS_submit_info;
        std::unique_ptr<vk::TimelineSemaphoreSubmitInfo> xLAS_timeline_semaphore_info = std::make_unique<vk::TimelineSemaphoreSubmitInfo>();
        xLAS_submit_info.pNext = xLAS_timeline_semaphore_info.get();
        xLAS_submit_info.commandBufferCount = 1;
        xLAS_submit_info.pCommandBuffers = &xLAS_command_buffer;
        // xLAS wait
        auto xLAS_wait_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto xLAS_wait_pipeline_stages = std::make_unique<std::vector<vk::PipelineStageFlags>>();
        auto xLAS_wait_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        xLAS_wait_semaphores->emplace_back(transformsFinishTimelineSemaphore);
        xLAS_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR);
        xLAS_wait_semaphores_values->emplace_back(frameCount);
        xLAS_submit_info.setWaitSemaphores(*xLAS_wait_semaphores);
        xLAS_submit_info.setWaitDstStageMask(*xLAS_wait_pipeline_stages);
        xLAS_timeline_semaphore_info->setWaitSemaphoreValues(*xLAS_wait_semaphores_values);
        // xLAS signal
        auto xLAS_signal_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto xLAS_signal_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        xLAS_signal_semaphores->emplace_back(xLASupdateFinishTimelineSemaphore);
        xLAS_signal_semaphores_values->emplace_back(frameCount);
        xLAS_submit_info.setSignalSemaphores(*xLAS_signal_semaphores);
        xLAS_timeline_semaphore_info->setSignalSemaphoreValues(*xLAS_signal_semaphores_values);

        // Push-back
        before_compute_submit_infos.emplace_back(xLAS_submit_info);
        timeline_semaphore_infos.emplace_back(std::move(xLAS_timeline_semaphore_info));
        semaphore_vectors.emplace_back(std::move(xLAS_wait_semaphores));
        pipelineStageFlags_vectors.emplace_back(std::move(xLAS_wait_pipeline_stages));
        semaphore_values_vectors.emplace_back(std::move(xLAS_wait_semaphores_values));
        semaphore_vectors.emplace_back(std::move(xLAS_signal_semaphores));
        semaphore_values_vectors.emplace_back(std::move(xLAS_signal_semaphores_values));
    }

    // Write host buffers
    WriteInitHostBuffers();

    // Get swapchain index and swap descriptor
    uint32_t swapchain_index = device.acquireNextImageKHR(graphics_ptr->GetSwapchain(),
                                                          0,
                                                          presentImageAvailableSemaphores[commandBuffer_index]).value;
    // Bind swapchain image to descriptor
    if (useMorphologicalAA)
        this->BindMAAimages(frameCount, swapchain_index);

    std::vector<vk::SubmitInfo> graphics_submit_infos;
    {
        FrustumCulling frustum_culling;
        frustum_culling.SetFrustumPlanes(viewport.GetWorldSpacePlanesOfFrustum());

        vk::CommandBuffer& graphics_command_buffer = graphicsCommandBuffers[commandBuffer_index];
        graphics_command_buffer.reset();
        RecordGraphicsCommandBuffer(graphics_command_buffer,
                                    swapchain_index,
                                    frustum_culling);

        vk::SubmitInfo graphics_submit_info;
        std::unique_ptr<vk::TimelineSemaphoreSubmitInfo> graphics_timeline_semaphore_info = std::make_unique<vk::TimelineSemaphoreSubmitInfo>();
        graphics_submit_info.pNext = graphics_timeline_semaphore_info.get();
        graphics_submit_info.commandBufferCount = 1;
        graphics_submit_info.pCommandBuffers = &graphics_command_buffer;
        // Graphics wait
        auto graphics_wait_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto graphics_wait_pipeline_stages = std::make_unique<std::vector<vk::PipelineStageFlags>>();
        auto graphics_wait_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        graphics_wait_semaphores->emplace_back(transformsFinishTimelineSemaphore);
        graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eVertexInput);
        graphics_wait_semaphores_values->emplace_back(frameCount);
        graphics_wait_semaphores->emplace_back(xLASupdateFinishTimelineSemaphore);
        graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eFragmentShader);
        graphics_wait_semaphores_values->emplace_back(frameCount);
        graphics_wait_semaphores->emplace_back(histogramFinishTimelineSemaphore);
        graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eComputeShader);
        graphics_wait_semaphores_values->emplace_back(std::max(int64_t(frameCount - 2), int64_t(0)));
        graphics_wait_semaphores->emplace_back(presentImageAvailableSemaphores[commandBuffer_index]);
        graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        graphics_wait_semaphores_values->emplace_back(0);
        graphics_submit_info.setWaitSemaphores(*graphics_wait_semaphores);
        graphics_submit_info.setWaitDstStageMask(*graphics_wait_pipeline_stages);
        graphics_timeline_semaphore_info->setWaitSemaphoreValues(*graphics_wait_semaphores_values);
        // Graphics signal
        auto graphics_signal_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto graphics_signal_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        graphics_signal_semaphores->emplace_back(graphicsFinishTimelineSemaphore);
        graphics_signal_semaphores_values->emplace_back(frameCount);
        graphics_signal_semaphores->emplace_back(readyForPresentSemaphores[commandBuffer_index]);
        graphics_signal_semaphores_values->emplace_back(0);
        graphics_submit_info.setSignalSemaphores(*graphics_signal_semaphores);
        graphics_timeline_semaphore_info->setSignalSemaphoreValues(*graphics_signal_semaphores_values);

        // Push-back
        graphics_submit_infos.emplace_back(graphics_submit_info);
        timeline_semaphore_infos.emplace_back(std::move(graphics_timeline_semaphore_info));
        semaphore_vectors.emplace_back(std::move(graphics_wait_semaphores));
        pipelineStageFlags_vectors.emplace_back(std::move(graphics_wait_pipeline_stages));
        semaphore_values_vectors.emplace_back(std::move(graphics_wait_semaphores_values));
        semaphore_vectors.emplace_back(std::move(graphics_signal_semaphores));
        semaphore_values_vectors.emplace_back(std::move(graphics_signal_semaphores_values));
    }

    std::vector<vk::SubmitInfo> after_compute_submit_infos;
    {
        vk::CommandBuffer& exposure_command_buffer = exposureCommandBuffers[commandBuffer_index];
        exposure_command_buffer.reset();
        exposure_command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        exposure_uptr->ObtainImageOwnership(exposure_command_buffer, frameCount % 2, vk::ImageLayout::eGeneral, graphicsQueue.second);
        exposure_uptr->RecordFrameHistogram(exposure_command_buffer, frameCount % 2, 1, FP16factor);
        exposure_uptr->TransferImageOwnership(exposure_command_buffer, frameCount % 2, vk::ImageLayout::eGeneral, graphicsQueue.second);

        exposure_command_buffer.end();

        vk::SubmitInfo exposure_submit_info;
        std::unique_ptr<vk::TimelineSemaphoreSubmitInfo> exposure_timeline_semaphore_info = std::make_unique<vk::TimelineSemaphoreSubmitInfo>();
        exposure_submit_info.pNext = exposure_timeline_semaphore_info.get();
        exposure_submit_info.commandBufferCount = 1;
        exposure_submit_info.pCommandBuffers = &exposure_command_buffer;
        // exposure wait
        auto exposure_wait_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto exposure_wait_pipeline_stages = std::make_unique<std::vector<vk::PipelineStageFlags>>();
        auto exposure_wait_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        exposure_wait_semaphores->emplace_back(graphicsFinishTimelineSemaphore);
        exposure_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eComputeShader);
        exposure_wait_semaphores_values->emplace_back(frameCount);
        exposure_submit_info.setWaitSemaphores(*exposure_wait_semaphores);
        exposure_submit_info.setWaitDstStageMask(*exposure_wait_pipeline_stages);
        exposure_timeline_semaphore_info->setWaitSemaphoreValues(*exposure_wait_semaphores_values);
        // exposure signal
        auto exposure_signal_semaphores = std::make_unique<std::vector<vk::Semaphore>>();
        auto exposure_signal_semaphores_values = std::make_unique<std::vector<uint64_t>>();
        exposure_signal_semaphores->emplace_back(histogramFinishTimelineSemaphore);
        exposure_signal_semaphores_values->emplace_back(frameCount);
        exposure_submit_info.setSignalSemaphores(*exposure_signal_semaphores);
        exposure_timeline_semaphore_info->setSignalSemaphoreValues(*exposure_signal_semaphores_values);

        after_compute_submit_infos.emplace_back(exposure_submit_info);
        timeline_semaphore_infos.emplace_back(std::move(exposure_timeline_semaphore_info));
        semaphore_vectors.emplace_back(std::move(exposure_wait_semaphores));
        pipelineStageFlags_vectors.emplace_back(std::move(exposure_wait_pipeline_stages));
        semaphore_values_vectors.emplace_back(std::move(exposure_wait_semaphores_values));
        semaphore_vectors.emplace_back(std::move(exposure_signal_semaphores));
        semaphore_values_vectors.emplace_back(std::move(exposure_signal_semaphores_values));
    }

    // Submit!
    if (before_compute_submit_infos.size())
        meshComputeQueue.first.submit(before_compute_submit_infos);
    graphicsQueue.first.submit(graphics_submit_infos);
    if (after_compute_submit_infos.size())
        exposureComputeQueue.first.submit(after_compute_submit_infos);

    // Present
    vk::PresentInfoKHR present_info;
    vk::SwapchainKHR swapchain = graphics_ptr->GetSwapchain();
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &readyForPresentSemaphores[commandBuffer_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &swapchain_index;

    graphicsQueue.first.presentKHR(present_info);
}


void RealtimeRenderer::RecordGraphicsCommandBuffer(vk::CommandBuffer command_buffer,
                                                   uint32_t swapchain_index,
                                                   const FrustumCulling &frustum_culling)
{
    command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Obtain ownerships
    std::vector<vk::BufferMemoryBarrier> ownership_obtain_buffer_barriers;
    std::vector<vk::ImageMemoryBarrier> ownership_obtain_image_barriers;
    for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericTransformRangesBarriers(drawDynamicMeshInfos, frameCount)) {
        this_barrier.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eShaderRead;
        this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_obtain_buffer_barriers.emplace_back(this_barrier);
    }
    for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericBLASrangesBarriers(drawDynamicMeshInfos, frameCount)) {
        this_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_obtain_buffer_barriers.emplace_back(this_barrier);
    }
    {
        auto this_barrier = TLASbuilder_uptr->GetGenericTLASrangesBarrier(frameCount);
        this_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_obtain_buffer_barriers.emplace_back(this_barrier);
    }
    if (frameCount > 2) {
        auto this_barrier = exposure_uptr->GetGenericImageBarrier(frameCount);
        this_barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
        this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_obtain_image_barriers.emplace_back(this_barrier);
    }

    if (ownership_obtain_buffer_barriers.size()
     || ownership_obtain_image_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                       vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       ownership_obtain_buffer_barriers,
                                       ownership_obtain_image_barriers);
    }

    // Render begin
    vk::RenderPassBeginInfo render_pass_begin_info;
    vk::ClearValue clear_values[9];
    render_pass_begin_info.renderPass = renderpass;
    render_pass_begin_info.framebuffer = frameBuffer;
    render_pass_begin_info.renderArea.offset = vk::Offset2D(0, 0);
    render_pass_begin_info.renderArea.extent = graphics_ptr->GetSwapchainCreateInfo().imageExtent;

    std::array<uint32_t, 4> visibility_clear = {0, 0, 0, 0};
    std::array<float, 4> color_clear = {0.0f, 0.0f, 0.f, 0.f};
    clear_values[0].color.uint32 = visibility_clear;
    clear_values[1].depthStencil.depth = 1.f;
    clear_values[8].color.float32 = color_clear;

    render_pass_begin_info.clearValueCount = 9;
    render_pass_begin_info.pClearValues = clear_values;

    command_buffer.beginRenderPass2(render_pass_begin_info, vk::SubpassContents::eInline);

    // Visibility pass
    {
        std::vector<DrawInfo> visibility_draw;
        std::copy(drawStaticMeshInfos.begin(), drawStaticMeshInfos.end(), std::back_inserter(visibility_draw));
        std::copy(drawDynamicMeshInfos.begin(), drawDynamicMeshInfos.end(), std::back_inserter(visibility_draw));
        for (const DrawInfo &this_draw: visibility_draw) {
            struct DrawPrimitiveInfo {
                DrawPrimitiveInfo(size_t in_primitiveIndex,
                                  PrimitiveInfo in_primitiveInfo,
                                  DynamicMeshInfo::DynamicPrimitiveInfo in_dynamicPrimitiveInfo,
                                  vk::Buffer in_dynamicBuffer,
                                  uint32_t in_dynamicBufferRangeSize)
                        :
                        primitiveIndex(in_primitiveIndex),
                        primitiveInfo(in_primitiveInfo),
                        dynamicPrimitiveInfo(in_dynamicPrimitiveInfo),
                        dynamicBuffer(in_dynamicBuffer),
                        dynamicBufferRangeSize(in_dynamicBufferRangeSize) {}

                size_t primitiveIndex;

                PrimitiveInfo primitiveInfo;

                DynamicMeshInfo::DynamicPrimitiveInfo dynamicPrimitiveInfo;
                vk::Buffer dynamicBuffer;
                uint32_t dynamicBufferRangeSize;
            };
            std::vector<DrawPrimitiveInfo> draw_primitives_infos;

            if (this_draw.dynamicMeshIndex != -1) {
                const auto &dynamic_mesh_info = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw.dynamicMeshIndex);
                for (const auto &this_dynamic_primitive_info: dynamic_mesh_info.dynamicPrimitives) {
                    const PrimitiveInfo &this_primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);
                    draw_primitives_infos.emplace_back(this_dynamic_primitive_info.primitiveIndex,
                                                       this_primitive_info,
                                                       this_dynamic_primitive_info,
                                                       dynamic_mesh_info.buffer,
                                                       dynamic_mesh_info.rangeSize);
                }
            } else {
                for (size_t primitive_index: graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw.meshIndex).primitivesIndex) {
                    const PrimitiveInfo &primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(primitive_index);
                    draw_primitives_infos.emplace_back(primitive_index,
                                                       primitive_info,
                                                       DynamicMeshInfo::DynamicPrimitiveInfo(),
                                                       vk::Buffer(),
                                                       -1);
                }
            }

            for (size_t i = 0; i != draw_primitives_infos.size(); ++i) {
                const auto &this_draw_primitive_info = draw_primitives_infos[i];

                const MaterialAbout &this_material = graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialAbout(this_draw_primitive_info.primitiveInfo.material);

                vk::Pipeline pipeline = primitivesPipelines[this_draw_primitive_info.primitiveIndex];
                command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

                vk::PipelineLayout pipeline_layout = primitivesPipelineLayouts[this_draw_primitive_info.primitiveIndex];
                std::vector<vk::DescriptorSet> descriptor_sets;
                descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(frameCount));
                descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(frameCount));
                if (this_material.masked)
                    descriptor_sets.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSet());

                command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                                  pipeline_layout,
                                                  0,
                                                  descriptor_sets,
                                                  {});

                std::array<uint32_t, 1> data_vertex = {uint32_t(this_draw.matricesOffset)};
                command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, 4, data_vertex.data());

                std::array<uint32_t, 2> data_frag = {uint32_t(this_draw.primitivesInstanceOffset + i), uint32_t(this_draw_primitive_info.primitiveInfo.material)};
                if (not this_material.masked)
                    command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 4, 4, data_frag.data());
                else
                    command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 4, 8, data_frag.data());

                vk::Buffer static_primitives_buffer = graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer();
                std::vector<vk::Buffer> buffers;
                std::vector<vk::DeviceSize> offsets;

                // Position
                if (this_draw_primitive_info.dynamicPrimitiveInfo.positionByteOffset != -1) {
                    offsets.emplace_back(this_draw_primitive_info.dynamicPrimitiveInfo.positionByteOffset + (frameCount % 3) * this_draw_primitive_info.dynamicBufferRangeSize);
                    buffers.emplace_back(this_draw_primitive_info.dynamicBuffer);
                } else {
                    offsets.emplace_back(this_draw_primitive_info.primitiveInfo.positionByteOffset);
                    buffers.emplace_back(static_primitives_buffer);
                }

                // Color texcoords if material is masked
                if (this_material.masked) {
                    if (this_draw_primitive_info.dynamicPrimitiveInfo.texcoordsByteOffset != -1) {
                        offsets.emplace_back(this_draw_primitive_info.dynamicPrimitiveInfo.texcoordsByteOffset + this_material.color_texcooord * sizeof(glm::vec2)
                                             + (frameCount % 3) * this_draw_primitive_info.dynamicBufferRangeSize);
                        buffers.emplace_back(this_draw_primitive_info.dynamicBuffer);
                    } else {
                        offsets.emplace_back(this_draw_primitive_info.primitiveInfo.texcoordsByteOffset + this_material.color_texcooord * sizeof(glm::vec2));
                        buffers.emplace_back(static_primitives_buffer);
                    }
                }

                command_buffer.bindVertexBuffers(0, buffers, offsets);

                command_buffer.bindIndexBuffer(graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer(),
                                               this_draw_primitive_info.primitiveInfo.indicesByteOffset,
                                               vk::IndexType::eUint32);

                command_buffer.drawIndexed(uint32_t(this_draw_primitive_info.primitiveInfo.indicesCount), 1, 0, 0, 0);
            }
        }
    }

    command_buffer.nextSubpass2({vk::SubpassContents::eInline}, {});

    // Path trace pass
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pathTracePipeline);

        std::vector<vk::DescriptorSet> descriptor_sets;
        descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(frameCount));
        descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(frameCount));
        descriptor_sets.emplace_back(graphics_ptr->GetDynamicMeshes()->GetDescriptorSet());
        descriptor_sets.emplace_back(graphics_ptr->GetDynamicMeshes()->GetPrevDescriptorSet());
        descriptor_sets.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSet());
        descriptor_sets.emplace_back(hostDescriptorSets[frameCount % 3]);
        descriptor_sets.emplace_back(pathTraceDescriptorSet);
        descriptor_sets.emplace_back(TLASbuilder_uptr->GetDescriptorSet(frameCount));
        descriptor_sets.emplace_back(graphics_ptr->GetLights()->GetDescriptorSet());

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          pathTracePipelineLayout,
                                          0,
                                          descriptor_sets,
                                          {});

        struct push_constants_type{
            std::array<glm::vec4,1> vec4_constants = {};
            std::array<uint32_t, 5> uint_constants = {};
            std::array<float, 1> float_constants = {};
        } push_constants;

        push_constants.vec4_constants = {glm::vec4(graphics_ptr->GetLights()->GetUniformLuminance(), 0.f)};
        push_constants.uint_constants = {graphics_ptr->GetSwapchainCreateInfo().imageExtent.width,
                                         graphics_ptr->GetSwapchainCreateInfo().imageExtent.height,
                                         (uint32_t)frameCount,
                                         coneLightsIndicesRange.offset,
                                         coneLightsIndicesRange.size};
        push_constants.float_constants = {FP16factor};

        command_buffer.pushConstants(pathTracePipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, sizeof(push_constants_type), &push_constants);

        std::vector<vk::Buffer> buffers;
        std::vector<vk::DeviceSize> offsets;

        buffers.emplace_back(fullscreenBuffer);
        offsets.emplace_back((frameCount % 3) * fullscreenBufferPartSize);

        command_buffer.bindVertexBuffers(0, buffers, offsets);

        command_buffer.draw(3, 1, 0, 0);
    }

    command_buffer.nextSubpass2({vk::SubpassContents::eInline}, {});

    // Light sources draw
    {
        // Sky draw
        {
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, lightDrawPipeline);

            std::vector<vk::DescriptorSet> descriptor_sets;
            descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(frameCount));
            descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(frameCount));

            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                              lightDrawPipelineLayout,
                                              0,
                                              descriptor_sets,
                                              {});

            struct push_VS_constants_type {
                std::array<uint32_t, 3> uint_constants = {};
            } push_VS_constants;

            struct push_FS_constants_type {
                std::array<glm::vec4, 1> vec4_constants = {};
            } push_FS_constants;

            push_VS_constants.uint_constants = {0, 0, 1};
            command_buffer.pushConstants(lightDrawPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_VS_constants_type), &push_VS_constants);

            push_FS_constants.vec4_constants = {glm::vec4(graphics_ptr->GetLights()->GetUniformLuminance() / FP16factor, 1.f)};
            command_buffer.pushConstants(lightDrawPipelineLayout, vk::ShaderStageFlagBits::eFragment, 16, sizeof(push_FS_constants_type), &push_FS_constants);

            std::vector<vk::Buffer> buffers;
            std::vector<vk::DeviceSize> offsets;

            buffers.emplace_back(fullscreenBuffer);
            offsets.emplace_back((frameCount % 3) * fullscreenBufferPartSize);

            command_buffer.draw(3, 1, 0, 0);
        }

        // etc light sources
        {
            // Pipeline and descriptor already binded

            std::vector<DrawInfo> light_draw;
            std::copy(drawDirectionalLightSources.begin(), drawDirectionalLightSources.end(), std::back_inserter(light_draw));
            std::copy(drawLocalLightSources.begin(), drawLocalLightSources.end(), std::back_inserter(light_draw));
            for (const DrawInfo &this_draw: light_draw) {
                for (size_t primitive_index: graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw.meshIndex).primitivesIndex) {
                    const PrimitiveInfo &primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(primitive_index);
                    const LightInfo &light_info = graphics_ptr->GetLights()->GetLightInfo(this_draw.lightIndex);

                    struct push_VS_constants_type {
                        std::array<uint32_t, 3> uint_constants = {};
                    } push_VS_constants;

                    struct push_FS_constants_type {
                        std::array<glm::vec4, 1> vec4_constants = {};
                    } push_FS_constants;

                    push_VS_constants.uint_constants = {uint32_t(this_draw.matricesOffset),
                                                        uint32_t(light_info.lightType == LightType::Cone),
                                                        0};
                    command_buffer.pushConstants(lightDrawPipelineLayout, vk::ShaderStageFlagBits::eVertex, 0, sizeof(push_VS_constants_type), &push_VS_constants);

                    push_FS_constants.vec4_constants = {glm::vec4(light_info.luminance / FP16factor, 1.f)};
                    command_buffer.pushConstants(lightDrawPipelineLayout, vk::ShaderStageFlagBits::eFragment, 16, sizeof(push_FS_constants_type), &push_FS_constants);

                    vk::Buffer static_primitives_buffer = graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer();
                    std::vector<vk::Buffer> buffers;
                    std::vector<vk::DeviceSize> offsets;

                    // Position
                    offsets.emplace_back(primitive_info.positionByteOffset);
                    buffers.emplace_back(static_primitives_buffer);

                    command_buffer.bindVertexBuffers(0, buffers, offsets);

                    command_buffer.bindIndexBuffer(graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer(),
                                                   primitive_info.indicesByteOffset,
                                                   vk::IndexType::eUint32);

                    command_buffer.drawIndexed(uint32_t(primitive_info.indicesCount), 1, 0, 0, 0);

                }
            }
        }
    }

    command_buffer.endRenderPass2(vk::SubpassEndInfo());

    // Denoise
    NRDintegration_uptr->Denoise(command_buffer);

    // Resolve compute
    { // Dispatch compute
        uint32_t width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        uint32_t height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;

        std::vector<vk::DescriptorSet> descriptor_sets;
        descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(frameCount));
        descriptor_sets.emplace_back(resolveDescriptorSets[frameCount % 2]);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, resolveCompPipelineLayout, 0, descriptor_sets, {});

        struct push_constants_type{
            std::array<uint32_t, 2> uint_constants = {};
            std::array<float, 1> float_constants = {};
            std::array<glm::vec4, 3> vec4_constants = {};
        } push_constants;
        push_constants.uint_constants = {width,
                                         height};
        push_constants.float_constants = {exposure_uptr->GetCurrectScale() * FP16factor};
        push_constants.vec4_constants = viewport.GetFullscreenpassTriangleNormals();
        command_buffer.pushConstants(resolveCompPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(push_constants_type), &push_constants);

        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, resolveCompPipeline);

        uint32_t groups_count_x = (width + comp_dim_size - 1) / comp_dim_size;
        uint32_t groups_count_y = (height + comp_dim_size - 1) / comp_dim_size;
        command_buffer.dispatch(groups_count_x, groups_count_y, 1);
    }

    if (useMorphologicalAA) {
        {   // swapchain image: none -> storage
            vk::ImageMemoryBarrier image_memory_barrier;
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
            image_memory_barrier.oldLayout = vk::ImageLayout::eUndefined;
            image_memory_barrier.newLayout = vk::ImageLayout::eGeneral;
            image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barrier.image = graphics_ptr->GetSwapchainImages()[swapchain_index];
            image_memory_barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor,
                                                      0, 1,
                                                      0, 1};

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                           vk::PipelineStageFlagBits::eComputeShader,
                                           vk::DependencyFlagBits::eByRegion,
                                           0, nullptr,
                                           0, nullptr,
                                           1, &image_memory_barrier);
        }

        {   // morphological AA dispatch
            uint32_t width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
            uint32_t height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;

            std::vector<vk::DescriptorSet> descriptor_sets;
            descriptor_sets.emplace_back(morphologicalAAdescriptorSets[frameCount % 3]);
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, morphologicalAAcompPipelineLayout, 0, descriptor_sets, {});

            std::array<uint32_t, 2> uint_push_constants = {width, height};
            command_buffer.pushConstants(morphologicalAAcompPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint_push_constants), uint_push_constants.data());

            command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, morphologicalAAcompPipeline);

            uint32_t groups_count_x = (width + comp_dim_size - 1) / comp_dim_size;
            uint32_t groups_count_y = (height + comp_dim_size - 1) / comp_dim_size;
            command_buffer.dispatch(groups_count_x, groups_count_y, 1);
        }

        {   // swapchain image: storage -> present
            vk::ImageMemoryBarrier image_memory_barrier;
            image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
            image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eNone;
            image_memory_barrier.oldLayout = vk::ImageLayout::eGeneral;
            image_memory_barrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
            image_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barrier.image = graphics_ptr->GetSwapchainImages()[swapchain_index];
            image_memory_barrier.subresourceRange = { vk::ImageAspectFlagBits::eColor,
                                                      0, 1,
                                                      0, 1};

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                           vk::PipelineStageFlagBits::eAllCommands,
                                           vk::DependencyFlagBits::eByRegion,
                                           0, nullptr,
                                           0, nullptr,
                                           1, &image_memory_barrier);
        }

    } else {
        {   // prepare for copy
            vk::ImageMemoryBarrier image_memory_barriers[2];

            image_memory_barriers[0].srcAccessMask = vk::AccessFlagBits::eNone;
            image_memory_barriers[0].dstAccessMask = vk::AccessFlagBits::eTransferWrite;
            image_memory_barriers[0].oldLayout = vk::ImageLayout::eUndefined;
            image_memory_barriers[0].newLayout = vk::ImageLayout::eTransferDstOptimal;
            image_memory_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[0].image = graphics_ptr->GetSwapchainImages()[swapchain_index];
            image_memory_barriers[0].subresourceRange = { vk::ImageAspectFlagBits::eColor,
                                                          0, 1,
                                                          0, 1};

            image_memory_barriers[1].srcAccessMask = vk::AccessFlagBits::eShaderWrite;
            image_memory_barriers[1].dstAccessMask = vk::AccessFlagBits::eTransferRead;
            image_memory_barriers[1].oldLayout = vk::ImageLayout::eGeneral;
            image_memory_barriers[1].newLayout = vk::ImageLayout::eTransferSrcOptimal;
            image_memory_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[1].image = resolveResultImage;
            image_memory_barriers[1].subresourceRange = { vk::ImageAspectFlagBits::eColor,
                                                          0, 1,
                                                          0, 1};

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                           vk::PipelineStageFlagBits::eTransfer,
                                           vk::DependencyFlagBits::eByRegion,
                                           0, nullptr,
                                           0, nullptr,
                                           2, image_memory_barriers);
        }

        {   // copy
            vk::ImageCopy image_copy_region;
            image_copy_region.srcSubresource = { vk::ImageAspectFlagBits::eColor,
                                                 0,0,1};
            image_copy_region.srcOffset = VkOffset3D{0, 0};
            image_copy_region.dstSubresource = { vk::ImageAspectFlagBits::eColor,
                                                 0,0,1};
            image_copy_region.dstOffset = VkOffset3D{0, 0};
            image_copy_region.extent = VkExtent3D{graphics_ptr->GetSwapchainCreateInfo().imageExtent.width,
                                                  graphics_ptr->GetSwapchainCreateInfo().imageExtent.height,
                                                  1};

            command_buffer.copyImage(resolveResultImage,
                                     vk::ImageLayout::eTransferSrcOptimal,
                                     graphics_ptr->GetSwapchainImages()[swapchain_index],
                                     vk::ImageLayout::eTransferDstOptimal,
                                     1, &image_copy_region);
        }

        {   // present and revert result to image
            vk::ImageMemoryBarrier image_memory_barriers[2];

            image_memory_barriers[0].srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            image_memory_barriers[0].dstAccessMask = vk::AccessFlagBits::eNone;
            image_memory_barriers[0].oldLayout = vk::ImageLayout::eTransferDstOptimal;
            image_memory_barriers[0].newLayout = vk::ImageLayout::ePresentSrcKHR;
            image_memory_barriers[0].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[0].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[0].image = graphics_ptr->GetSwapchainImages()[swapchain_index];
            image_memory_barriers[0].subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
            image_memory_barriers[0].subresourceRange = { vk::ImageAspectFlagBits::eColor,
                                                      0, 1,
                                                      0, 1};

            image_memory_barriers[1].srcAccessMask = vk::AccessFlagBits::eTransferRead;
            image_memory_barriers[1].dstAccessMask = vk::AccessFlagBits::eNone;
            image_memory_barriers[1].oldLayout = vk::ImageLayout::eTransferSrcOptimal;
            image_memory_barriers[1].newLayout = vk::ImageLayout::eGeneral;
            image_memory_barriers[1].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[1].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            image_memory_barriers[1].image = resolveResultImage;
            image_memory_barriers[1].subresourceRange = { vk::ImageAspectFlagBits::eColor,
                                                          0, 1,
                                                          0, 1};

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                           vk::PipelineStageFlagBits::eAllCommands,
                                           vk::DependencyFlagBits::eByRegion,
                                           0, nullptr,
                                           0, nullptr,
                                           2, image_memory_barriers);
        }
    }

    // Transfer ownership
    std::vector<vk::BufferMemoryBarrier> ownership_transfer_memory_barriers;
    std::vector<vk::ImageMemoryBarrier> ownership_transfer_image_barriers;

    for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericTransformRangesBarriers(drawDynamicMeshInfos, frameCount)) {
        this_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
        this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_transfer_memory_barriers.emplace_back(this_barrier);
    }
    for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericBLASrangesBarriers(drawDynamicMeshInfos, frameCount)) {
        this_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_transfer_memory_barriers.emplace_back(this_barrier);
    }
    {
        auto this_barrier = TLASbuilder_uptr->GetGenericTLASrangesBarrier(frameCount);
        this_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_transfer_memory_barriers.emplace_back(this_barrier);
    }

    {
        auto this_barrier = exposure_uptr->GetGenericImageBarrier(frameCount);
        this_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
        if (this_barrier.srcQueueFamilyIndex != this_barrier.dstQueueFamilyIndex)
            ownership_transfer_image_barriers.emplace_back(this_barrier);
    }

    if (ownership_transfer_memory_barriers.size()
     || ownership_transfer_image_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                       vk::PipelineStageFlagBits::eBottomOfPipe,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       ownership_transfer_memory_barriers,
                                       ownership_transfer_image_barriers);
    }

    command_buffer.end();
}

void RealtimeRenderer::WriteInitHostBuffers() const
{
    graphics_ptr->WriteCameraMarticesBuffers(viewport,
                                             matrices,
                                             drawInfos,
                                             frameCount);

    TLASbuilder_uptr->WriteHostInstanceBuffer(TLAS_instances,
                                              frameCount);

    graphics_ptr->GetLights()->WriteLightsBuffers();

    uint32_t buffer_index = frameCount % 3;
    {
        memcpy((std::byte *) (primitivesInstanceAllocInfo.pMappedData) + buffer_index * primitivesInstanceBufferPartSize,
               primitive_instance_parameters.data(),
               primitive_instance_parameters.size() * sizeof(PrimitiveInstanceParameters));
        vma_allocator.flushAllocation(primitivesInstanceAllocation,
                                      buffer_index * primitivesInstanceBufferPartSize,
                                      primitive_instance_parameters.size() * sizeof(PrimitiveInstanceParameters));
    }
    {
        std::array<std::array<glm::vec4, 3>, 2> vertex_data = {viewport.GetFullscreenpassTrianglePos(),
                                                               viewport.GetFullscreenpassTriangleNormals()};

        memcpy((std::byte *) (fullscreenAllocInfo.pMappedData) + buffer_index * fullscreenBufferPartSize,
               vertex_data.data(),
               fullscreenBufferPartSize);
        vma_allocator.flushAllocation(fullscreenAllocation,
                                      buffer_index * fullscreenBufferPartSize,
                                      fullscreenBufferPartSize);
    }
}

void RealtimeRenderer::AssortDrawInfos()
{
    drawDynamicMeshInfos.clear();
    drawStaticMeshInfos.clear();
    drawLocalLightSources.clear();
    drawDirectionalLightSources.clear();
    for(const auto& draw_info : drawInfos) {
        if (draw_info.isLightSource) {
            const LightInfo& light_info = graphics_ptr->GetLights()->GetLightInfo(draw_info.lightIndex);
            if (light_info.lightType != LightType::Cone)
                drawLocalLightSources.emplace_back(draw_info);
            else
                drawDirectionalLightSources.emplace_back(draw_info);
        } else {
            if (draw_info.dynamicMeshIndex != -1)
                drawDynamicMeshInfos.emplace_back(draw_info);
            else
                drawStaticMeshInfos.emplace_back(draw_info);
        }
    }

    // Sort local light sources back to front
    std::sort(drawLocalLightSources.begin(), drawLocalLightSources.end(), [this](const DrawInfo& light_a, const DrawInfo& light_b)
    {
        auto light_a_pos = glm::vec3(matrices[light_a.matricesOffset].positionMatrix[3]);
        auto light_b_pos = glm::vec3(matrices[light_b.matricesOffset].positionMatrix[3]);
        return glm::length(light_a_pos) > glm::length(light_b_pos);
    });
}

void RealtimeRenderer::BindMAAimages(uint32_t frame_index, uint32_t swapchain_index)
{
    uint32_t buffer_index = frame_index % 3;

    vk::WriteDescriptorSet write_descriptor_sets[1];
    vk::DescriptorImageInfo descriptor_image_infos[1];

    descriptor_image_infos[0].imageLayout = vk::ImageLayout::eGeneral;
    descriptor_image_infos[0].imageView = graphics_ptr->GetSwapchainImageViews()[swapchain_index];
    descriptor_image_infos[0].sampler = nullptr;

    write_descriptor_sets[0].dstSet = morphologicalAAdescriptorSets[buffer_index];
    write_descriptor_sets[0].dstBinding = 10;
    write_descriptor_sets[0].dstArrayElement = 0;
    write_descriptor_sets[0].descriptorCount = 1;
    write_descriptor_sets[0].descriptorType = vk::DescriptorType::eStorageImage;
    write_descriptor_sets[0].pImageInfo = &descriptor_image_infos[0];

    device.updateDescriptorSets(1, write_descriptor_sets,
                                0, nullptr);
}

void RealtimeRenderer::PrepareNRDsettings()
{
    glm::mat4 transform_matrix = {1.f, 0.f, 0.f, 0.f,
                                  0.f,-1.f, 0.f, 0.f,
                                  0.f, 0.f, 1.f, 0.f,
                                  0.f, 0.f, 0.f, 1.f};


    glm::mat4 view_to_clip = viewport.GetPerspectiveMatrix();
    glm::mat4 prev_view_to_clip = prevFrameViewport.GetPerspectiveMatrix();
    glm::mat4 world_to_view = transform_matrix * viewport.GetViewMatrix();
    glm::mat4 prev_world_to_view = transform_matrix * prevFrameViewport.GetViewMatrix();
    memcpy(NRD_commonSettings.viewToClipMatrix, &view_to_clip, sizeof(glm::mat4));
    memcpy(NRD_commonSettings.viewToClipMatrixPrev, &prev_view_to_clip, sizeof(glm::mat4));
    memcpy(NRD_commonSettings.worldToViewMatrix, &world_to_view, sizeof(glm::mat4));
    memcpy(NRD_commonSettings.worldToViewMatrixPrev, &prev_world_to_view, sizeof(glm::mat4));
    if (frameCount < 4)
        NRD_commonSettings.accumulationMode = nrd::AccumulationMode::CLEAR_AND_RESTART;
    else
        NRD_commonSettings.accumulationMode = nrd::AccumulationMode::CONTINUE;

    NRD_commonSettings.frameIndex = frameCount - 1;
}
