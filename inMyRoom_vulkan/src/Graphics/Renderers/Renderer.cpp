#include "Graphics/Renderers/Renderer.h"

#include "Graphics/Graphics.h"
#include "Graphics/HelperUtils.h"

#include <iostream>

Renderer::Renderer(class Graphics* in_graphics_ptr,
                   vk::Device in_device,
                   vma::Allocator in_vma_allocator)
    : RendererBase(in_graphics_ptr, in_device, in_vma_allocator),
      graphicsQueue(graphics_ptr->GetQueuesList().graphicsQueues[0]),
      meshComputeQueue(graphics_ptr->GetQueuesList().dedicatedComputeQueues[0]),
      exposureComputeQueue(graphics_ptr->GetQueuesList().dedicatedComputeQueues[1])
{
    InitBuffers();
    InitImages();
    InitExposure();
    InitTLAS();
    InitDescriptors();
    InitRenderpasses();
    InitFramebuffers();
    InitSemaphoresAndFences();
    InitCommandBuffers();
    InitPrimitivesSet();
    InitShadePipeline();
    InitToneMapPipeline();
}

Renderer::~Renderer()
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

    for (size_t i = 0; i != 2; ++i) {
        for (auto &this_framebuffer: framebuffers[i]) {
            device.destroy(this_framebuffer);
        }
    }

    device.destroy(renderpass);

    device.destroy(depthImageView);
    vma_allocator.destroyImage(depthImage, depthAllocation);

    device.destroy(visibilityImageView);
    vma_allocator.destroyImage(visibilityImage, visibilityAllocation);

    TLASbuilder_uptr.reset();
    exposure_uptr.reset();

    device.destroy(photometricResultImageViews[0]);
    vma_allocator.destroyImage(photometricResultImages[0], photometricResultAllocations[0]);
    device.destroy(photometricResultImageViews[1]);
    vma_allocator.destroyImage(photometricResultImages[1], photometricResultAllocations[1]);

    device.destroy(descriptorPool);
    device.destroy(rendererDescriptorSetLayout);
    device.destroy(hostDescriptorSetLayout);

    vma_allocator.destroyBuffer(primitivesInstanceBuffer, primitivesInstanceAllocation);
    vma_allocator.destroyBuffer(fullscreenBuffer, fullscreenAllocation);
}

void Renderer::InitBuffers()
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
        fullscreenBufferPartSize = sizeof(glm::vec4) * 8 ;

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

void Renderer::InitImages()
{
    // z-buffer
    {
        depthImageCreateInfo.imageType = vk::ImageType::e2D;
        depthImageCreateInfo.format = vk::Format::eD32Sfloat;
        depthImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        depthImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        depthImageCreateInfo.extent.depth = 1;
        depthImageCreateInfo.mipLevels = 1;
        depthImageCreateInfo.arrayLayers = 1;
        depthImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
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

    // visibility image
    {
        visibilityImageCreateInfo.imageType = vk::ImageType::e2D;
        visibilityImageCreateInfo.format = vk::Format::eR32G32Uint;
        visibilityImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        visibilityImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        visibilityImageCreateInfo.extent.depth = 1;
        visibilityImageCreateInfo.mipLevels = 1;
        visibilityImageCreateInfo.arrayLayers = 1;
        visibilityImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
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

    {   // photometricResult image
        photometricResultImageCreateInfo.imageType = vk::ImageType::e2D;
        photometricResultImageCreateInfo.format = vk::Format::eR32G32B32A32Sfloat;
        photometricResultImageCreateInfo.extent.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        photometricResultImageCreateInfo.extent.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        photometricResultImageCreateInfo.extent.depth = 1;
        photometricResultImageCreateInfo.mipLevels = 1;
        photometricResultImageCreateInfo.arrayLayers = 1;
        photometricResultImageCreateInfo.samples = vk::SampleCountFlagBits::e1;
        photometricResultImageCreateInfo.tiling = vk::ImageTiling::eOptimal;
        photometricResultImageCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment
                | vk::ImageUsageFlagBits::eInputAttachment
                | vk::ImageUsageFlagBits::eStorage;
        photometricResultImageCreateInfo.sharingMode = vk::SharingMode::eExclusive;
        photometricResultImageCreateInfo.initialLayout = vk::ImageLayout::eUndefined;

        vma::AllocationCreateInfo image_allocation_info;
        image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

        for (size_t i = 0; i != 2; ++i) {
            auto createImage_result = vma_allocator.createImage(photometricResultImageCreateInfo, image_allocation_info).value;
            photometricResultImages[i] = createImage_result.first;
            photometricResultAllocations[i] = createImage_result.second;

            vk::ImageViewCreateInfo imageView_create_info;
            imageView_create_info.image = photometricResultImages[i];
            imageView_create_info.viewType = vk::ImageViewType::e2D;
            imageView_create_info.format = photometricResultImageCreateInfo.format;
            imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity,
                                                vk::ComponentSwizzle::eIdentity};
            imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                      0, 1,
                                                      0, 1};

            photometricResultImageViews[i] = device.createImageView(imageView_create_info).value;

            // Initialize
            OneShotCommandBuffer one_shot_command_buffer(device);
            vk::CommandBuffer command_buffer = one_shot_command_buffer.BeginCommandRecord(graphicsQueue);

            vk::ImageMemoryBarrier init_image_barrier;
            init_image_barrier.image = photometricResultImages[i];
            init_image_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            init_image_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
            init_image_barrier.oldLayout = vk::ImageLayout::eUndefined;
            init_image_barrier.newLayout = vk::ImageLayout::eGeneral;
            init_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            init_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            init_image_barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                   0, 1, 0, 1};

            command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe,
                                           vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                           vk::DependencyFlagBits::eByRegion,
                                           0, nullptr,
                                           0, nullptr,
                                           1, &init_image_barrier);

            one_shot_command_buffer.EndAndSubmitCommands();
        }
    }
}

void Renderer::InitExposure()
{
    std::tuple<vk::Image, vk::ImageView, vk::ImageCreateInfo> images_tuples[2] = {std::make_tuple(photometricResultImages[0], photometricResultImageViews[0], photometricResultImageCreateInfo),
                                                                                  std::make_tuple(photometricResultImages[1], photometricResultImageViews[1], photometricResultImageCreateInfo)};
    exposure_uptr = std::make_unique<Exposure>(device, vma_allocator, graphics_ptr, images_tuples, exposureComputeQueue);
}

void Renderer::InitTLAS()
{
    TLASbuilder_uptr = std::make_unique<TLASbuilder>(device, vma_allocator, meshComputeQueue.second, graphics_ptr->GetMaxInstancesCount());
}


void Renderer::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 3);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eInputAttachment, 2 * 2);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2 + 3,
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
    {   // Create renderer layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Input attachment
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 0;
            attach_binding.descriptorType = vk::DescriptorType::eInputAttachment;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(attach_binding);
        }
        {   // Photometric result
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 1;
            attach_binding.descriptorType = vk::DescriptorType::eInputAttachment;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(attach_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        rendererDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
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
    {   // Allocate renderer sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(rendererDescriptorSetLayout);
        layouts.emplace_back(rendererDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        rendererDescriptorSets[0] = descriptor_sets[0];
        rendererDescriptorSets[1] = descriptor_sets[1];
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
    {   // Write descriptors of renderer sets
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;
        std::vector<std::unique_ptr<vk::DescriptorImageInfo>> descriptor_image_infos_uptrs;
        std::vector<std::unique_ptr<vk::WriteDescriptorSetAccelerationStructureKHR>> acceleration_structures_pnext_uptrs;

        for (size_t i = 0; i != 2; ++i) {
            {
                auto descriptor_image_info_uptr = std::make_unique<vk::DescriptorImageInfo>();
                descriptor_image_info_uptr->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                descriptor_image_info_uptr->imageView = visibilityImageView;
                descriptor_image_info_uptr->sampler = nullptr;

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = rendererDescriptorSets[i];
                write_descriptor_set.dstBinding = 0;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eInputAttachment;
                write_descriptor_set.pImageInfo = descriptor_image_info_uptr.get();

                descriptor_image_infos_uptrs.emplace_back(std::move(descriptor_image_info_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
            }
            {
                auto descriptor_image_info_uptr = std::make_unique<vk::DescriptorImageInfo>();
                descriptor_image_info_uptr->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                descriptor_image_info_uptr->imageView = photometricResultImageViews[i];
                descriptor_image_info_uptr->sampler = nullptr;

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = rendererDescriptorSets[i];
                write_descriptor_set.dstBinding = 1;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eInputAttachment;
                write_descriptor_set.pImageInfo = descriptor_image_info_uptr.get();

                descriptor_image_infos_uptrs.emplace_back(std::move(descriptor_image_info_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
            }
        }

        device.updateDescriptorSets(writes_descriptor_set, {});
    }

}

void Renderer::InitRenderpasses()
{
    // Attachments
    vk::AttachmentDescription attachment_descriptions[4];

    attachment_descriptions[0].format = visibilityImageCreateInfo.format;
    attachment_descriptions[0].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[0].storeOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[0].stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[0].initialLayout = vk::ImageLayout::eUndefined;
    attachment_descriptions[0].finalLayout = vk::ImageLayout::eShaderReadOnlyOptimal;

    attachment_descriptions[1].format = depthImageCreateInfo.format;
    attachment_descriptions[1].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[1].loadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[1].storeOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[1].stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[1].initialLayout = vk::ImageLayout::eUndefined;
    attachment_descriptions[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    attachment_descriptions[2].format = photometricResultImageCreateInfo.format;
    attachment_descriptions[2].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[2].loadOp = vk::AttachmentLoadOp::eLoad;
    attachment_descriptions[2].storeOp = vk::AttachmentStoreOp::eStore;
    attachment_descriptions[2].stencilLoadOp = vk::AttachmentLoadOp::eLoad;
    attachment_descriptions[2].stencilStoreOp = vk::AttachmentStoreOp::eStore;
    attachment_descriptions[2].initialLayout = vk::ImageLayout::eGeneral;
    attachment_descriptions[2].finalLayout = vk::ImageLayout::eGeneral;

    attachment_descriptions[3].format = graphics_ptr->GetSwapchainCreateInfo().imageFormat;
    attachment_descriptions[3].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[3].loadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[3].storeOp = vk::AttachmentStoreOp::eStore;
    attachment_descriptions[3].stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[3].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[3].initialLayout = vk::ImageLayout::eUndefined;
    attachment_descriptions[3].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // Subpasses
    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::SubpassDependency> dependencies;

    // Subpass 0 (visibility)
    std::vector<vk::AttachmentReference> visibility_colorAttachments_refs;
    {
        vk::AttachmentReference visibilityAttach_ref;
        visibilityAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        visibilityAttach_ref.attachment = 0;
        visibility_colorAttachments_refs.emplace_back(visibilityAttach_ref);
    }

    vk::AttachmentReference visibility_depthAttach_ref;
    visibility_depthAttach_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    visibility_depthAttach_ref.attachment = 1;

    vk::SubpassDescription visibility_subpass_description;
    visibility_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    visibility_subpass_description.setColorAttachments(visibility_colorAttachments_refs);
    visibility_subpass_description.pDepthStencilAttachment = &visibility_depthAttach_ref;

    vk::SubpassDependency visibility_subpass_dependency;
    visibility_subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    visibility_subpass_dependency.dstSubpass = 0;
    visibility_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eAllGraphics;
    visibility_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    visibility_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    visibility_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

    subpasses.emplace_back(visibility_subpass_description);
    dependencies.emplace_back(visibility_subpass_dependency);

    // Subpass 1 (shading)
    std::vector<vk::AttachmentReference> shade_colorAttachments_refs;
    {
        vk::AttachmentReference colorAttach_ref;
        colorAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttach_ref.attachment = 2;
        shade_colorAttachments_refs.emplace_back(colorAttach_ref);
    }

    std::vector<vk::AttachmentReference> shade_inputAttachments_refs;
    {
        vk::AttachmentReference inputAttach_ref;
        inputAttach_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        inputAttach_ref.attachment = 0;
        shade_inputAttachments_refs.emplace_back(inputAttach_ref);
    }

    vk::SubpassDescription shade_subpass_description;
    shade_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    shade_subpass_description.setColorAttachments(shade_colorAttachments_refs);
    shade_subpass_description.setInputAttachments(shade_inputAttachments_refs);

    vk::SubpassDependency shade_subpass_dependency;
    shade_subpass_dependency.srcSubpass = 0;
    shade_subpass_dependency.dstSubpass = 1;
    shade_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    shade_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    shade_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    shade_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;
    shade_subpass_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpasses.emplace_back(shade_subpass_description);
    dependencies.emplace_back(shade_subpass_dependency);

    // Subpass 2 (tonemap)
    std::vector<vk::AttachmentReference> toneMap_colorAttachments_refs;
    {
        vk::AttachmentReference colorAttach_ref;
        colorAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttach_ref.attachment = 3;
        toneMap_colorAttachments_refs.emplace_back(colorAttach_ref);
    }

    std::vector<vk::AttachmentReference> toneMap_inputAttachments_refs;
    {
        vk::AttachmentReference inputAttach_ref;
        inputAttach_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        inputAttach_ref.attachment = 2;
        toneMap_inputAttachments_refs.emplace_back(inputAttach_ref);
    }

    vk::SubpassDescription toneMap_subpass_description;
    toneMap_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    toneMap_subpass_description.setColorAttachments(toneMap_colorAttachments_refs);
    toneMap_subpass_description.setInputAttachments(toneMap_inputAttachments_refs);

    vk::SubpassDependency toneMap_subpass_dependency;
    toneMap_subpass_dependency.srcSubpass = 1;
    toneMap_subpass_dependency.dstSubpass = 2;
    toneMap_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    toneMap_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    toneMap_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    toneMap_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;
    toneMap_subpass_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpasses.emplace_back(toneMap_subpass_description);
    dependencies.emplace_back(toneMap_subpass_dependency);

    // Renderpass
    vk::RenderPassCreateInfo renderpass_create_info;
    renderpass_create_info.attachmentCount = 4;
    renderpass_create_info.pAttachments = attachment_descriptions;
    renderpass_create_info.setSubpasses(subpasses);
    renderpass_create_info.setDependencies(dependencies);

    renderpass = device.createRenderPass(renderpass_create_info).value;
}

void Renderer::InitFramebuffers()
{
    for (size_t i = 0; i != 2; ++i) {
        for (const auto &swapchain_imageview: graphics_ptr->GetSwapchainImageViews()) {
            std::vector<vk::ImageView> attachments;
            attachments.emplace_back(visibilityImageView);
            attachments.emplace_back(depthImageView);
            attachments.emplace_back(photometricResultImageViews[i]);
            attachments.emplace_back(swapchain_imageview);

            vk::FramebufferCreateInfo framebuffer_createInfo;
            framebuffer_createInfo.renderPass = renderpass;
            framebuffer_createInfo.setAttachments(attachments);
            framebuffer_createInfo.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
            framebuffer_createInfo.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
            framebuffer_createInfo.layers = 1;

            framebuffers[i].emplace_back(device.createFramebuffer(framebuffer_createInfo).value);
        }
    }
}


void Renderer::InitSemaphoresAndFences()
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

void Renderer::InitCommandBuffers()
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

void Renderer::InitPrimitivesSet()
{
    // Create primitives sets (shaders-pipelines for each kind of primitive)
    printf("-Initializing \"Texture Pass\" primitives set\n");
    for(size_t i = 0; i != graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitivesCount(); ++i)
    {
        const PrimitiveInfo& this_primitiveInfo = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(i);
        const MaterialAbout& this_material = graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialAbout(this_primitiveInfo.material);

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

            // TODO? tesselation
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

            ShadersSpecs shaders_specs {"Visibility Shaders", shadersDefinitionStringPairs};
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

void Renderer::InitShadePipeline()
{
    printf("-Initializing \"Shade Pass\" pipeline\n");

    std::vector<std::pair<std::string, std::string>> shadersDefinitionStringPairs;
    shadersDefinitionStringPairs.emplace_back("TEXTURES_COUNT", std::to_string(graphics_ptr->GetTexturesOfMaterials()->GetTexturesCount()));
    shadersDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
    shadersDefinitionStringPairs.emplace_back("INSTANCES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
    shadersDefinitionStringPairs.emplace_back("MATERIALS_PARAMETERS_COUNT", std::to_string(graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialsCount()));

    {   // Pipeline layout fullscreen
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetCameraDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMatricesDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetDynamicMeshes()->GetDescriptorLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSetLayout());
        descriptor_sets_layouts.emplace_back(hostDescriptorSetLayout);
        descriptor_sets_layouts.emplace_back(rendererDescriptorSetLayout);
        descriptor_sets_layouts.emplace_back(TLASbuilder_uptr->GetDescriptorSetLayout());
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 0, 4 * 4);

        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        fullscreenPipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
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
                                                       uint32_t(4 * sizeof(float)),
                                                       vk::VertexInputRate::eVertex);
        vertex_input_attribute_descriptions.emplace_back(location_index++, binding_index,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         0);
        vertex_input_attribute_descriptions.emplace_back(location_index++, binding_index,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         uint32_t(4 * 4 * sizeof(float)));

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
        vk::PipelineColorBlendAttachmentState color_blend_attachment;
        color_blend_attachment.colorWriteMask = vk::ColorComponentFlagBits::eR | vk::ColorComponentFlagBits::eG |
                                                vk::ColorComponentFlagBits::eB | vk::ColorComponentFlagBits::eA;
        color_blend_attachment.blendEnable = VK_TRUE;
        color_blend_attachment.srcColorBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.dstColorBlendFactor = vk::BlendFactor::eOneMinusSrcAlpha;
        color_blend_attachment.colorBlendOp = vk::BlendOp::eAdd;
        color_blend_attachment.srcAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.dstAlphaBlendFactor = vk::BlendFactor::eOne;
        color_blend_attachment.alphaBlendOp = vk::BlendOp::eAdd;

        color_blend_create_info.logicOpEnable = VK_FALSE;
        color_blend_create_info.attachmentCount = 1;
        color_blend_create_info.pAttachments = &color_blend_attachment;

        pipeline_create_info.pColorBlendState = &color_blend_create_info;

        // PipelineShaderStageCreateInfo
        std::vector<vk::PipelineShaderStageCreateInfo> shaders_stage_create_infos;

        ShadersSpecs shaders_specs {"Shade-Pass Shaders", shadersDefinitionStringPairs};
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
        pipeline_create_info.layout = fullscreenPipelineLayout;
        pipeline_create_info.renderPass = renderpass;
        pipeline_create_info.subpass = 1;

        fullscreenPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(pipeline_create_info).first;
    }
}

void Renderer::InitToneMapPipeline()
{
    printf("-Initializing \"Tone-Map Pass\" pipeline\n");

    std::vector<std::pair<std::string, std::string>> shadersDefinitionStringPairs;
    shadersDefinitionStringPairs.emplace_back("INPUT_ATTACHMENT_SET", std::to_string(0));
    shadersDefinitionStringPairs.emplace_back("INPUT_ATTACHMENT_BIND", std::to_string(1));

    {   // Pipeline layout tone map
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(rendererDescriptorSetLayout);
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 0, 4);

        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        toneMapPipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
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
                                                       uint32_t(4 * sizeof(float)),
                                                       vk::VertexInputRate::eVertex);
        vertex_input_attribute_descriptions.emplace_back(location_index++, binding_index,
                                                         vk::Format::eR32G32B32A32Sfloat,
                                                         0);

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

        ShadersSpecs shaders_specs {"ToneMap-Pass Shaders", shadersDefinitionStringPairs};
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
        pipeline_create_info.layout = toneMapPipelineLayout;
        pipeline_create_info.renderPass = renderpass;
        pipeline_create_info.subpass = 2;

        toneMapPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(pipeline_create_info).first;
    }
}

void Renderer::DrawFrame(ViewportFrustum in_viewport,
                         std::vector<ModelMatrices>&& in_matrices,
                         std::vector<DrawInfo>&& in_draw_infos)
{
    ++frameCount;
    if(viewportFreeze) {
        if (viewportFreezeState == ViewportFreezeStates::ready) {
            viewportFreezeState = ViewportFreezeStates::next_frame_freeze;
        } else if (viewportFreezeState == ViewportFreezeStates::next_frame_freeze
           || viewportFreezeState == ViewportFreezeStates::frozen) {
            viewportFreezeState = ViewportFreezeStates::frozen;
        }
    } else {
        if (viewportFreezeState == ViewportFreezeStates::ready
         || viewportFreezeState == ViewportFreezeStates::next_frame_unfreeze) {
            viewportFreezeState = ViewportFreezeStates::ready;
        } else if (viewportFreezeState == ViewportFreezeStates::frozen) {
            viewportFreezeState = ViewportFreezeStates::next_frame_unfreeze;
        }
    }

    if (viewportFreezeState == ViewportFreezeStates::ready
      || viewportFreezeState == ViewportFreezeStates::next_frame_freeze) {
        viewport = in_viewport;
        matrices = std::move(in_matrices);
        drawInfos = std::move(in_draw_infos);
        viewportInRowFreezedFrameCount = 1;

        drawDynamicMeshInfos.clear();
        for(const auto& draw_info : drawInfos) {
            if (draw_info.dynamicMeshIndex != -1)
                drawDynamicMeshInfos.emplace_back(draw_info);
        }
    } else {
        ++viewportFreezedFrameCount;
        ++viewportInRowFreezedFrameCount;
    }

    size_t hostvisible_freezeable_buffer_index = (frameCount - viewportFreezedFrameCount) % 3;
    size_t hostvisible_buffer_index = frameCount % 3;
    size_t device_freezeable_buffer_index = (frameCount - viewportFreezedFrameCount) % 2;
    size_t device_buffer_index = frameCount % 2;

    size_t freezable_commandBuffer_index = (frameCount - viewportFreezedFrameCount) % 3;
    size_t commandBuffer_index = frameCount % 3;

    //
    // Wait! Wait for write buffers (-3)
    if (frameCount > 3) {
        uint64_t wait_value = uint64_t(frameCount - 3);

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

    std::vector<vk::SubmitInfo> before_compute_submit_infos;
    if (viewportFreezeState == ViewportFreezeStates::ready
     || viewportFreezeState == ViewportFreezeStates::next_frame_freeze) {
        graphics_ptr->GetDynamicMeshes()->SwapDescriptorSet(frameCount - viewportFreezedFrameCount);
        primitive_instance_parameters = CreatePrimitivesInstanceParameters();
        TLAS_instances = TLASbuilder::CreateTLASinstances(drawInfos, matrices, device_freezeable_buffer_index, graphics_ptr);

        {
            vk::CommandBuffer &transform_command_buffer = transformCommandBuffers[freezable_commandBuffer_index];
            transform_command_buffer.reset();
            transform_command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            if (frameCount > 2) graphics_ptr->GetDynamicMeshes()->ObtainTransformRanges(transform_command_buffer, drawDynamicMeshInfos, graphicsQueue.second);
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

            before_compute_submit_infos.emplace_back(transform_submit_info);
            timeline_semaphore_infos.emplace_back(std::move(transform_timeline_semaphore_info));
            semaphore_vectors.emplace_back(std::move(transform_wait_semaphores));
            pipelineStageFlags_vectors.emplace_back(std::move(transform_wait_pipeline_stages));
            semaphore_values_vectors.emplace_back(std::move(transform_wait_semaphores_values));
            semaphore_vectors.emplace_back(std::move(transform_signal_semaphores));
            semaphore_values_vectors.emplace_back(std::move(transform_signal_semaphores_values));
        }
        {
            vk::CommandBuffer &xLAS_command_buffer = xLASCommandBuffers[freezable_commandBuffer_index];
            xLAS_command_buffer.reset();
            xLAS_command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

            if (frameCount > 2) graphics_ptr->GetDynamicMeshes()->ObtainBLASranges(xLAS_command_buffer, drawDynamicMeshInfos, graphicsQueue.second);
            graphics_ptr->GetDynamicMeshes()->RecordBLASupdate(xLAS_command_buffer, drawDynamicMeshInfos);
            if (frameCount > 2) TLASbuilder_uptr->ObtainTLASranges(xLAS_command_buffer, device_freezeable_buffer_index, graphicsQueue.second);
            TLASbuilder_uptr->RecordTLASupdate(xLAS_command_buffer, hostvisible_freezeable_buffer_index, device_freezeable_buffer_index, TLAS_instances.size());
            graphics_ptr->GetDynamicMeshes()->TransferTransformAndBLASranges(xLAS_command_buffer, drawDynamicMeshInfos, graphicsQueue.second);
            TLASbuilder_uptr->TransferTLASrange(xLAS_command_buffer, device_freezeable_buffer_index, graphicsQueue.second);

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
            xLAS_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eComputeShader);
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

            before_compute_submit_infos.emplace_back(xLAS_submit_info);
            timeline_semaphore_infos.emplace_back(std::move(xLAS_timeline_semaphore_info));
            semaphore_vectors.emplace_back(std::move(xLAS_wait_semaphores));
            pipelineStageFlags_vectors.emplace_back(std::move(xLAS_wait_pipeline_stages));
            semaphore_values_vectors.emplace_back(std::move(xLAS_wait_semaphores_values));
            semaphore_vectors.emplace_back(std::move(xLAS_signal_semaphores));
            semaphore_values_vectors.emplace_back(std::move(xLAS_signal_semaphores_values));
        }

        // Write host buffers
        WriteInitHostBuffers(hostvisible_freezeable_buffer_index);
    }

    std::vector<vk::SubmitInfo> after_compute_submit_infos;
    {   // Exposure
        exposure_uptr->GetNextFrameValue(frameCount);

        vk::CommandBuffer& exposure_command_buffer = exposureCommandBuffers[commandBuffer_index];
        exposure_command_buffer.reset();
        exposure_command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

        exposure_uptr->ObtainImageOwnership(exposure_command_buffer, device_freezeable_buffer_index, vk::ImageLayout::eGeneral, graphicsQueue.second);
        exposure_uptr->RecordFrameHistogram(exposure_command_buffer, device_freezeable_buffer_index, 1.f / float(viewportInRowFreezedFrameCount));
        exposure_uptr->TransferImageOwnership(exposure_command_buffer, device_freezeable_buffer_index, vk::ImageLayout::eGeneral, graphicsQueue.second);

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
       // exposure_wait_semaphores->emplace_back(transformsFinishTimelineSemaphore);
       // exposure_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eComputeShader);
       // exposure_wait_semaphores_values->emplace_back(frameCount + 1);
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

    uint32_t swapchain_index = device.acquireNextImageKHR(graphics_ptr->GetSwapchain(),
                                                          0,
                                                          presentImageAvailableSemaphores[commandBuffer_index]).value;

    std::vector<vk::SubmitInfo> graphics_submit_infos;
    {
        FrustumCulling frustum_culling;
        frustum_culling.SetFrustumPlanes(viewport.GetWorldSpacePlanesOfFrustum());

        vk::CommandBuffer& graphics_command_buffer = graphicsCommandBuffers[commandBuffer_index];
        graphics_command_buffer.reset();
        RecordGraphicsCommandBuffer(graphics_command_buffer,
                                    uint32_t(hostvisible_freezeable_buffer_index),
                                    uint32_t(device_freezeable_buffer_index),
                                    uint32_t(device_buffer_index),
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
        if (viewportFreezeState == ViewportFreezeStates::ready
         || viewportFreezeState == ViewportFreezeStates::next_frame_freeze) {
            graphics_wait_semaphores->emplace_back(xLASupdateFinishTimelineSemaphore);
            graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eTopOfPipe);
            graphics_wait_semaphores_values->emplace_back(frameCount);
        }
        graphics_wait_semaphores->emplace_back(presentImageAvailableSemaphores[commandBuffer_index]);
        graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        graphics_wait_semaphores_values->emplace_back(0);
        graphics_wait_semaphores->emplace_back(histogramFinishTimelineSemaphore);
        graphics_wait_pipeline_stages->emplace_back(vk::PipelineStageFlagBits::eColorAttachmentOutput);
        if (viewportFreezeState == ViewportFreezeStates::ready
            || viewportFreezeState == ViewportFreezeStates::next_frame_freeze) {
            graphics_wait_semaphores_values->emplace_back(std::max(int64_t(frameCount - 2), int64_t(0)));
        } else {
            graphics_wait_semaphores_values->emplace_back(std::max(int64_t(frameCount - 1), int64_t(0)));
        }
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

        graphics_submit_infos.emplace_back(graphics_submit_info);
        timeline_semaphore_infos.emplace_back(std::move(graphics_timeline_semaphore_info));
        semaphore_vectors.emplace_back(std::move(graphics_wait_semaphores));
        pipelineStageFlags_vectors.emplace_back(std::move(graphics_wait_pipeline_stages));
        semaphore_values_vectors.emplace_back(std::move(graphics_wait_semaphores_values));
        semaphore_vectors.emplace_back(std::move(graphics_signal_semaphores));
        semaphore_values_vectors.emplace_back(std::move(graphics_signal_semaphores_values));
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

void Renderer::RecordGraphicsCommandBuffer(vk::CommandBuffer command_buffer,
                                           uint32_t freezable_host_buffer_index,
                                           uint32_t freezable_device_buffer_index,
                                           uint32_t device_buffer_index,
                                           uint32_t swapchain_index,
                                           const FrustumCulling& frustum_culling)
{
    command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Obtain ownerships
    std::vector<vk::BufferMemoryBarrier> ownership_obtain_buffer_barriers;
    std::vector<vk::ImageMemoryBarrier> ownership_obtain_image_barriers;
    if (viewportFreezeState == ViewportFreezeStates::ready
     || viewportFreezeState == ViewportFreezeStates::next_frame_freeze) {
        for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericTransformRangesBarriers(drawDynamicMeshInfos, freezable_device_buffer_index)) {
            this_barrier.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eShaderRead;
            this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
            ownership_obtain_buffer_barriers.emplace_back(this_barrier);
        }
        for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericBLASrangesBarriers(drawDynamicMeshInfos, freezable_device_buffer_index)) {
            this_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
            this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
            ownership_obtain_buffer_barriers.emplace_back(this_barrier);
        }
        {
            auto this_barrier = TLASbuilder_uptr->GetGenericTLASrangesBarrier(freezable_device_buffer_index);
            this_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
            this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
            ownership_obtain_buffer_barriers.emplace_back(this_barrier);
        }
    }
    if (frameCount > 2) {
        auto this_barrier = exposure_uptr->GetGenericImageBarrier(freezable_device_buffer_index);
        this_barrier.dstAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        this_barrier.dstQueueFamilyIndex = graphicsQueue.second;
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
    vk::ClearValue clear_values[4];
    render_pass_begin_info.renderPass = renderpass;
    render_pass_begin_info.framebuffer = framebuffers[freezable_device_buffer_index][swapchain_index];
    render_pass_begin_info.renderArea.offset = vk::Offset2D(0, 0);
    render_pass_begin_info.renderArea.extent = graphics_ptr->GetSwapchainCreateInfo().imageExtent;

    std::array<uint32_t, 4> visibility_clear = {0, 0, 0, 0};
    std::array<float, 4> color_clear = {0.0f, 0.0f, 0.f, 0.f};
    clear_values[0].color.uint32 = visibility_clear;
    clear_values[1].depthStencil.depth = 1.f;
    clear_values[2].color.float32 = color_clear;
    clear_values[3].color.float32 = color_clear;

    render_pass_begin_info.clearValueCount = 4;
    render_pass_begin_info.pClearValues = clear_values;

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    // Visibility pass
    for (const DrawInfo& this_draw: drawInfos) {
        struct DrawPrimitiveInfo {
            DrawPrimitiveInfo(size_t in_primitiveIndex,
                              PrimitiveInfo in_primitiveInfo,
                              DynamicMeshInfo::DynamicPrimitiveInfo in_dynamicPrimitiveInfo,
                              vk::Buffer in_dynamicBuffer,
                              uint32_t in_dynamicBufferHalfSize)
                :
                primitiveIndex(in_primitiveIndex),
                primitiveInfo(in_primitiveInfo),
                dynamicPrimitiveInfo(in_dynamicPrimitiveInfo),
                dynamicBuffer(in_dynamicBuffer),
                dynamicBufferHalfSize(in_dynamicBufferHalfSize) {}
            size_t primitiveIndex;

            PrimitiveInfo primitiveInfo;

            DynamicMeshInfo::DynamicPrimitiveInfo dynamicPrimitiveInfo;
            vk::Buffer dynamicBuffer;
            uint32_t dynamicBufferHalfSize;
        };
        std::vector<DrawPrimitiveInfo> draw_primitives_infos;

        if (this_draw.dynamicMeshIndex != -1) {
            const auto &dynamic_mesh_info = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw.dynamicMeshIndex);
            for (const auto& this_dynamic_primitive_info: dynamic_mesh_info.dynamicPrimitives) {
                const PrimitiveInfo &this_primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);
                draw_primitives_infos.emplace_back(this_dynamic_primitive_info.primitiveIndex,
                                                   this_primitive_info,
                                                   this_dynamic_primitive_info,
                                                   dynamic_mesh_info.buffer,
                                                   dynamic_mesh_info.halfSize);
            }
        } else {
            for (size_t primitive_index : graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw.meshIndex).primitivesIndex) {
                const PrimitiveInfo& primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(primitive_index);
                draw_primitives_infos.emplace_back(primitive_index,
                                                   primitive_info,
                                                   DynamicMeshInfo::DynamicPrimitiveInfo(),
                                                   vk::Buffer(),
                                                   -1);
            }
        }

        for (size_t i = 0; i != draw_primitives_infos.size(); ++i) {
            const auto& this_draw_primitive_info = draw_primitives_infos[i];

            const MaterialAbout& this_material = graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialAbout(this_draw_primitive_info.primitiveInfo.material);

            vk::Pipeline pipeline = primitivesPipelines[this_draw_primitive_info.primitiveIndex];
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

            vk::PipelineLayout pipeline_layout = primitivesPipelineLayouts[this_draw_primitive_info.primitiveIndex];
            std::vector<vk::DescriptorSet> descriptor_sets;
            descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(freezable_host_buffer_index));
            descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(freezable_host_buffer_index));
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
                offsets.emplace_back(this_draw_primitive_info.dynamicPrimitiveInfo.positionByteOffset + freezable_device_buffer_index * this_draw_primitive_info.dynamicBufferHalfSize);
                buffers.emplace_back(this_draw_primitive_info.dynamicBuffer);
            } else {
                offsets.emplace_back(this_draw_primitive_info.primitiveInfo.positionByteOffset);
                buffers.emplace_back(static_primitives_buffer);
            }

            // Color texcoords if material is masked
            if (this_material.masked) {
                if (this_draw_primitive_info.dynamicPrimitiveInfo.texcoordsByteOffset != -1) {
                    offsets.emplace_back(this_draw_primitive_info.dynamicPrimitiveInfo.texcoordsByteOffset + this_material.color_texcooord * sizeof(glm::vec2)
                                        + freezable_device_buffer_index * this_draw_primitive_info.dynamicBufferHalfSize );
                    buffers.emplace_back(this_draw_primitive_info.dynamicBuffer);
                } else {
                    offsets.emplace_back(this_draw_primitive_info.primitiveInfo.texcoordsByteOffset + this_material.color_texcooord * sizeof(glm::vec2) );
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

    command_buffer.nextSubpass(vk::SubpassContents::eInline);

    // Shade pass
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, fullscreenPipeline);

        std::vector<vk::DescriptorSet> descriptor_sets;
        descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(freezable_host_buffer_index));
        descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(freezable_host_buffer_index));
        descriptor_sets.emplace_back(graphics_ptr->GetDynamicMeshes()->GetDescriptorSet());
        descriptor_sets.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSet());
        descriptor_sets.emplace_back(hostDescriptorSets[freezable_host_buffer_index]);
        descriptor_sets.emplace_back(rendererDescriptorSets[freezable_device_buffer_index]);
        descriptor_sets.emplace_back(TLASbuilder_uptr->GetDescriptorSet(freezable_device_buffer_index));

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          fullscreenPipelineLayout,
                                          0,
                                          descriptor_sets,
                                          {});

        std::array<uint32_t, 4> push_constants = {graphics_ptr->GetSwapchainCreateInfo().imageExtent.width,
                                                  graphics_ptr->GetSwapchainCreateInfo().imageExtent.height,
                                                  uint32_t(frameCount),
                                                  viewportInRowFreezedFrameCount == 1 ? uint32_t(true) : uint32_t(false)};
        command_buffer.pushConstants(fullscreenPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, 4 * 4, push_constants.data());

        std::vector<vk::Buffer> buffers;
        std::vector<vk::DeviceSize> offsets;

        buffers.emplace_back(fullscreenBuffer);
        offsets.emplace_back(freezable_host_buffer_index * fullscreenBufferPartSize);

        command_buffer.bindVertexBuffers(0, buffers, offsets);

        command_buffer.draw(4, 1, 0, 0);
    }

    command_buffer.nextSubpass(vk::SubpassContents::eInline);

    // Tone map pass
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, toneMapPipeline);

        std::vector<vk::DescriptorSet> descriptor_sets;
        descriptor_sets.emplace_back(rendererDescriptorSets[freezable_device_buffer_index]);

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          toneMapPipelineLayout,
                                          0,
                                          descriptor_sets,
                                          {});

        std::array<float, 1> push_constants = { exposure_uptr->GetCurrectScale() / float(viewportInRowFreezedFrameCount) };
        command_buffer.pushConstants(toneMapPipelineLayout, vk::ShaderStageFlagBits::eFragment, 0, 4, push_constants.data());

        std::vector<vk::Buffer> buffers;
        std::vector<vk::DeviceSize> offsets;

        buffers.emplace_back(fullscreenBuffer);
        offsets.emplace_back(freezable_host_buffer_index * fullscreenBufferPartSize);

        command_buffer.bindVertexBuffers(0, buffers, offsets);

        command_buffer.draw(4, 1, 0, 0);
    }

    command_buffer.endRenderPass();

    // Tranfer ownerships
    std::vector<vk::BufferMemoryBarrier> ownership_transfer_memory_barriers;
    std::vector<vk::ImageMemoryBarrier> ownership_transfer_image_barriers;
    if ((viewportFreezeState == ViewportFreezeStates::ready
     || viewportFreezeState == ViewportFreezeStates::next_frame_unfreeze)) {
        for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericTransformRangesBarriers(drawDynamicMeshInfos, freezable_device_buffer_index)) {
            this_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
            ownership_transfer_memory_barriers.emplace_back(this_barrier);
        }
        for (auto this_barrier: graphics_ptr->GetDynamicMeshes()->GetGenericBLASrangesBarriers(drawDynamicMeshInfos, freezable_device_buffer_index)) {
            this_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
            this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
            ownership_transfer_memory_barriers.emplace_back(this_barrier);
        }
        {
            auto this_barrier = TLASbuilder_uptr->GetGenericTLASrangesBarrier(freezable_device_buffer_index);
            this_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
            this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
            ownership_transfer_memory_barriers.emplace_back(this_barrier);
        }
    }
    {
        auto this_barrier = exposure_uptr->GetGenericImageBarrier(freezable_device_buffer_index);
        this_barrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
        this_barrier.srcQueueFamilyIndex = graphicsQueue.second;
        ownership_transfer_image_barriers.emplace_back(this_barrier);
    }

    if (ownership_transfer_memory_barriers.size()
     || ownership_transfer_image_barriers.size())
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eColorAttachmentOutput,
                                   vk::PipelineStageFlagBits::eBottomOfPipe,
                                   vk::DependencyFlagBits::eByRegion,
                                   {},
                                   ownership_transfer_memory_barriers,
                                   ownership_transfer_image_barriers);

    command_buffer.end();
}

std::vector<Renderer::PrimitiveInstanceParameters> Renderer::CreatePrimitivesInstanceParameters()
{
    std::vector<PrimitiveInstanceParameters> return_vector;

    PrimitiveInstanceParameters default_instance_parameters = {};
    default_instance_parameters.material = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().material;
    default_instance_parameters.matricesOffset = 0;
    default_instance_parameters.indicesOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().indicesByteOffset / sizeof(uint32_t);
    default_instance_parameters.positionOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().positionByteOffset / sizeof(glm::vec4);
    default_instance_parameters.normalOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().normalByteOffset / sizeof(glm::vec4);
    default_instance_parameters.tangentOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().tangentByteOffset / sizeof(glm::vec4);
    default_instance_parameters.texcoordsOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().texcoordsByteOffset / sizeof(glm::vec2);
    default_instance_parameters.colorOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().colorByteOffset / sizeof(glm::vec4);
    return_vector.emplace_back(default_instance_parameters);

    for (DrawInfo& this_draw_info : drawInfos) {
        this_draw_info.primitivesInstanceOffset = return_vector.size();

        std::vector<PrimitiveInfo> primitives_info;
        std::vector<DynamicMeshInfo::DynamicPrimitiveInfo> dynamic_primitives_info;
        uint32_t descriptor_index = 0;
        if (this_draw_info.dynamicMeshIndex != -1) {
            descriptor_index = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex).descriptorIndexOffset + 1;
            dynamic_primitives_info = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex).dynamicPrimitives;
            for (const auto& this_dynamic_primitive_info: dynamic_primitives_info) {
                const PrimitiveInfo &this_primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);
                primitives_info.emplace_back(this_primitive_info);
            }
        } else {
            for (size_t primitive_index : graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw_info.meshIndex).primitivesIndex) {
                const PrimitiveInfo& primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(primitive_index);
                primitives_info.emplace_back(primitive_info);
                dynamic_primitives_info.emplace_back();
            }
        }

        for (size_t i = 0; i != primitives_info.size(); ++i) {
            PrimitiveInstanceParameters this_primitiveInstanceParameters = {};

            this_primitiveInstanceParameters.material = primitives_info[i].material;
            this_primitiveInstanceParameters.matricesOffset = this_draw_info.matricesOffset;

            if (primitives_info[i].drawMode == vk::PrimitiveTopology::eTriangleList)
                this_primitiveInstanceParameters.indicesSetMultiplier = 3;
            else if (primitives_info[i].drawMode == vk::PrimitiveTopology::eLineList)
                this_primitiveInstanceParameters.indicesSetMultiplier = 2;
            else
                this_primitiveInstanceParameters.indicesSetMultiplier = 1;
                
            this_primitiveInstanceParameters.indicesOffset = primitives_info[i].indicesByteOffset / sizeof(uint32_t);

            if (dynamic_primitives_info[i].positionByteOffset != -1) {
                this_primitiveInstanceParameters.positionOffset = dynamic_primitives_info[i].positionByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.positionDescriptorIndex = descriptor_index;
            } else {
                this_primitiveInstanceParameters.positionOffset = primitives_info[i].positionByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.positionDescriptorIndex = 0;
            }

            if (dynamic_primitives_info[i].normalByteOffset != -1) {
                this_primitiveInstanceParameters.normalOffset = dynamic_primitives_info[i].normalByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.normalDescriptorIndex = descriptor_index;
            } else {
                assert(primitives_info[i].normalByteOffset != -1);
                this_primitiveInstanceParameters.normalOffset = primitives_info[i].normalByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.normalDescriptorIndex = 0;
            }

            if (dynamic_primitives_info[i].tangentByteOffset != -1) {
                this_primitiveInstanceParameters.tangentOffset = dynamic_primitives_info[i].tangentByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.tangentDescriptorIndex = descriptor_index;
            } else {
                assert(primitives_info[i].tangentByteOffset != -1);
                this_primitiveInstanceParameters.tangentOffset = primitives_info[i].tangentByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.tangentDescriptorIndex = 0;
            }

            if (dynamic_primitives_info[i].texcoordsByteOffset != -1) {
                this_primitiveInstanceParameters.texcoordsStepMultiplier = dynamic_primitives_info[i].texcoordsCount;
                this_primitiveInstanceParameters.texcoordsOffset = dynamic_primitives_info[i].texcoordsByteOffset / sizeof(glm::vec2);
                this_primitiveInstanceParameters.texcoordsDescriptorIndex = descriptor_index;
            } else {
                if (primitives_info[i].texcoordsByteOffset != -1) {
                    this_primitiveInstanceParameters.texcoordsStepMultiplier = primitives_info[i].texcoordsCount;
                    this_primitiveInstanceParameters.texcoordsOffset = primitives_info[i].texcoordsByteOffset / sizeof(glm::vec2);
                    this_primitiveInstanceParameters.texcoordsDescriptorIndex = 0;
                } else {
                    this_primitiveInstanceParameters.texcoordsStepMultiplier = 0;
                    this_primitiveInstanceParameters.texcoordsOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().texcoordsByteOffset / sizeof(glm::vec2);
                    this_primitiveInstanceParameters.texcoordsDescriptorIndex = 0;
                }
            }

            if (dynamic_primitives_info[i].colorByteOffset != -1) {
                this_primitiveInstanceParameters.colorStepMultiplier = 1;
                this_primitiveInstanceParameters.colorOffset = dynamic_primitives_info[i].colorByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.colorDescriptorIndex = descriptor_index;
            } else {
                if (primitives_info[i].colorByteOffset != -1) {
                    this_primitiveInstanceParameters.colorStepMultiplier = 1;
                    this_primitiveInstanceParameters.colorOffset = primitives_info[i].colorByteOffset / sizeof(glm::vec4);
                    this_primitiveInstanceParameters.colorDescriptorIndex = 0;
                } else {
                    this_primitiveInstanceParameters.colorStepMultiplier = 0;
                    this_primitiveInstanceParameters.colorOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().colorByteOffset / sizeof(glm::vec4);
                    this_primitiveInstanceParameters.colorDescriptorIndex = 0;
                }
            }

            return_vector.emplace_back(this_primitiveInstanceParameters);
        }
    }

    return return_vector;
}

void Renderer::WriteInitHostBuffers(uint32_t buffer_index) const
{
    graphics_ptr->WriteCameraMarticesBuffers(viewport,
                                             matrices,
                                             drawInfos,
                                             buffer_index);

    TLASbuilder_uptr->WriteHostInstanceBuffer(TLAS_instances,
                                               buffer_index);

    {
        memcpy((std::byte *) (primitivesInstanceAllocInfo.pMappedData) + buffer_index * primitivesInstanceBufferPartSize,
               primitive_instance_parameters.data(),
               primitive_instance_parameters.size() * sizeof(PrimitiveInstanceParameters));
        vma_allocator.flushAllocation(primitivesInstanceAllocation,
                                      buffer_index * primitivesInstanceBufferPartSize,
                                      primitive_instance_parameters.size() * sizeof(PrimitiveInstanceParameters));
    }

    {
        std::array<std::array<glm::vec4, 4>, 2> vertex_data = {viewport.GetFullscreenpassTrianglePos(),
                                                               viewport.GetFullscreenpassTriangleNormals()};

        memcpy((std::byte *) (fullscreenAllocInfo.pMappedData) + buffer_index * fullscreenBufferPartSize,
               vertex_data.data(),
               fullscreenBufferPartSize);
        vma_allocator.flushAllocation(fullscreenAllocation,
                                      buffer_index * fullscreenBufferPartSize,
                                      fullscreenBufferPartSize);
    }
}
