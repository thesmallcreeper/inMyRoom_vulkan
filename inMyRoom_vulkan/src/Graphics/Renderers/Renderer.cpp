#include "Graphics/Renderers/Renderer.h"

#include "Graphics/Graphics.h"

Renderer::Renderer(class Graphics* in_graphics_ptr,
                   vk::Device in_device,
                   vma::Allocator in_vma_allocator)
    : RendererBase(in_graphics_ptr, in_device, in_vma_allocator)
{
    graphicsQueue = graphics_ptr->GetQueuesList().graphicsQueues[0];

    InitBuffers();
    InitImages();
    InitTLASes();
    InitDescriptors();
    InitRenderpasses();
    InitFramebuffers();
    InitSemaphoresAndFences();
    InitCommandBuffers();
    InitPrimitivesSet();
    InitFullscreenPipeline();
}

Renderer::~Renderer()
{
    device.destroy(commandPool);

    device.destroy(readyForPresentSemaphores[0]);
    device.destroy(readyForPresentSemaphores[1]);
    device.destroy(readyForPresentSemaphores[2]);
    device.destroy(presentImageAvailableSemaphores[0]);
    device.destroy(presentImageAvailableSemaphores[1]);
    device.destroy(presentImageAvailableSemaphores[2]);
    device.destroy(submitFinishTimelineSemaphore);
    device.destroy(hostWriteFinishTimelineSemaphore);

    for(auto& this_framebuffer : framebuffers) {
        device.destroy(this_framebuffer);
    }

    device.destroy(renderpass);

    device.destroy(depthImageView);
    vma_allocator.destroyImage(depthImage, depthAllocation);

    device.destroy(visibilityImageView);
    vma_allocator.destroyImage(visibilityImage, visibilityAllocation);

    device.destroy(descriptorPool);
    device.destroy(primitivesInstanceDescriptorSetLayout);

    vma_allocator.destroyBuffer(primitivesInstanceBuffer, primitivesInstanceAllocation);
    vma_allocator.destroyBuffer(fullscreenBuffer, fullscreenAllocation);

    device.destroy(TLASesHandles[0]);
    device.destroy(TLASesHandles[1]);
    vma_allocator.destroyBuffer(TLASesBuffer, TLASesAllocation);
    vma_allocator.destroyBuffer(TLASbuildScratchBuffer, TLASbuildScratchAllocation);
    vma_allocator.destroyBuffer(TLASesInstancesBuffer, TLASesInstancesAllocation);
}

void Renderer::InitBuffers()
{
    // primitivesInstanceBuffer
    {
        primitivesInstanceBufferHalfsize = sizeof(PrimitiveInstanceParameters) * graphics_ptr->GetMaxInstancesCount();
        primitivesInstanceBufferHalfsize += (primitivesInstanceBufferHalfsize % 16 == 0) ? 0 : 16 - primitivesInstanceBufferHalfsize % 16;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = primitivesInstanceBufferHalfsize * 2;
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
        fullscreenBufferHalfsize = sizeof(glm::vec4) * 8 ;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = fullscreenBufferHalfsize * 2;
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

    // TLASesInstancesBuffer
    {
        TLASesInstancesHalfSize = sizeof(vk::AccelerationStructureInstanceKHR) * graphics_ptr->GetMaxInstancesCount();

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = TLASesInstancesHalfSize * 2;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                | vk::BufferUsageFlagBits::eTransferDst
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              TLASesInstancesAllocInfo);

        assert(createBuffer_result.result == vk::Result::eSuccess);
        TLASesInstancesBuffer = createBuffer_result.value.first;
        TLASesInstancesAllocation = createBuffer_result.value.second;
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
}

void Renderer::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 2);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eAccelerationStructureKHR, 2);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eInputAttachment, 2);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }
    {   // Create primitivesInstanceSet layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Buffer
            vk::DescriptorSetLayoutBinding buffer_binding;
            buffer_binding.binding = 0;
            buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
            buffer_binding.descriptorCount = 1;
            buffer_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(buffer_binding);
        }
        {   // Input attachment
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 1;
            attach_binding.descriptorType = vk::DescriptorType::eInputAttachment;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(attach_binding);
        }
        {   // TLAS
            vk::DescriptorSetLayoutBinding TLAS_binding;
            TLAS_binding.binding = 2;
            TLAS_binding.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
            TLAS_binding.descriptorCount = 1;
            TLAS_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(TLAS_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        primitivesInstanceDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Allocate
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(primitivesInstanceDescriptorSetLayout);
        layouts.emplace_back(primitivesInstanceDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        primitivesInstanceDescriptorSets[0] = descriptor_sets[0];
        primitivesInstanceDescriptorSets[1] = descriptor_sets[1];
    }

    {   // Write descriptors
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;

        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;
        {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = primitivesInstanceBuffer;
            descriptor_buffer_info_uptr->offset = 0;
            descriptor_buffer_info_uptr->range  = primitivesInstanceBufferHalfsize;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = primitivesInstanceDescriptorSets[0];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }
        {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = primitivesInstanceBuffer;
            descriptor_buffer_info_uptr->offset = primitivesInstanceBufferHalfsize;
            descriptor_buffer_info_uptr->range  = primitivesInstanceBufferHalfsize;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = primitivesInstanceDescriptorSets[1];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        std::vector<std::unique_ptr<vk::DescriptorImageInfo>> descriptor_image_infos_uptrs;
        {
            auto descriptor_image_info_uptr = std::make_unique<vk::DescriptorImageInfo>();
            descriptor_image_info_uptr->imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
            descriptor_image_info_uptr->imageView = visibilityImageView;
            descriptor_image_info_uptr->sampler = nullptr;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = primitivesInstanceDescriptorSets[0];
            write_descriptor_set.dstBinding = 1;
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
            descriptor_image_info_uptr->imageView = visibilityImageView;
            descriptor_image_info_uptr->sampler = nullptr;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = primitivesInstanceDescriptorSets[1];
            write_descriptor_set.dstBinding = 1;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eInputAttachment;
            write_descriptor_set.pImageInfo = descriptor_image_info_uptr.get();

            descriptor_image_infos_uptrs.emplace_back(std::move(descriptor_image_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        std::vector<std::unique_ptr<vk::WriteDescriptorSetAccelerationStructureKHR>> acceleration_structures_pnext_uptrs;
        {
            auto acceleration_structures_pnext_uptr = std::make_unique<vk::WriteDescriptorSetAccelerationStructureKHR>();
            acceleration_structures_pnext_uptr->accelerationStructureCount = 1;
            acceleration_structures_pnext_uptr->pAccelerationStructures = &TLASesHandles[0];

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = primitivesInstanceDescriptorSets[0];
            write_descriptor_set.dstBinding = 2;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
            write_descriptor_set.pNext = acceleration_structures_pnext_uptr.get();

            acceleration_structures_pnext_uptrs.emplace_back(std::move(acceleration_structures_pnext_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }
        {
            auto acceleration_structures_pnext_uptr = std::make_unique<vk::WriteDescriptorSetAccelerationStructureKHR>();
            acceleration_structures_pnext_uptr->accelerationStructureCount = 1;
            acceleration_structures_pnext_uptr->pAccelerationStructures = &TLASesHandles[1];

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = primitivesInstanceDescriptorSets[1];
            write_descriptor_set.dstBinding = 2;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
            write_descriptor_set.pNext = acceleration_structures_pnext_uptr.get();

            acceleration_structures_pnext_uptrs.emplace_back(std::move(acceleration_structures_pnext_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        device.updateDescriptorSets(writes_descriptor_set, {});
    }
}

void Renderer::InitRenderpasses()
{
    // Attachments
    vk::AttachmentDescription attachment_descriptions[3];

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

    attachment_descriptions[2].format = graphics_ptr->GetSwapchainCreateInfo().imageFormat;
    attachment_descriptions[2].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[2].loadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[2].storeOp = vk::AttachmentStoreOp::eStore;
    attachment_descriptions[2].stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[2].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[2].initialLayout = vk::ImageLayout::eUndefined;
    attachment_descriptions[2].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    // Subpasses
    std::vector<vk::SubpassDescription> subpasses;
    std::vector<vk::SubpassDependency> dependencies;

    // Subpass 0
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
    visibility_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
    visibility_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    visibility_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
    visibility_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

    subpasses.emplace_back(visibility_subpass_description);
    dependencies.emplace_back(visibility_subpass_dependency);

    // Subpass 1
    std::vector<vk::AttachmentReference> texture_colorAttachments_refs;
    {
        vk::AttachmentReference colorAttach_ref;
        colorAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
        colorAttach_ref.attachment = 2;
        texture_colorAttachments_refs.emplace_back(colorAttach_ref);
    }

    std::vector<vk::AttachmentReference> texture_inputAttachments_refs;
    {
        vk::AttachmentReference inputAttach_ref;
        inputAttach_ref.layout = vk::ImageLayout::eShaderReadOnlyOptimal;
        inputAttach_ref.attachment = 0;
        texture_inputAttachments_refs.emplace_back(inputAttach_ref);
    }

    vk::SubpassDescription texture_subpass_description;
    texture_subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    texture_subpass_description.setColorAttachments(texture_colorAttachments_refs);
    texture_subpass_description.setInputAttachments(texture_inputAttachments_refs);

    vk::SubpassDependency texture_subpass_dependency;
    texture_subpass_dependency.srcSubpass = 0;
    texture_subpass_dependency.dstSubpass = 1;
    texture_subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eColorAttachmentOutput;
    texture_subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eFragmentShader;
    texture_subpass_dependency.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    texture_subpass_dependency.dstAccessMask = vk::AccessFlagBits::eInputAttachmentRead;
    texture_subpass_dependency.dependencyFlags = vk::DependencyFlagBits::eByRegion;

    subpasses.emplace_back(texture_subpass_description);
    dependencies.emplace_back(texture_subpass_dependency);

    // Renderpass
    vk::RenderPassCreateInfo renderpass_create_info;
    renderpass_create_info.attachmentCount = 3;
    renderpass_create_info.pAttachments = attachment_descriptions;
    renderpass_create_info.setSubpasses(subpasses);
    renderpass_create_info.setDependencies(dependencies);

    renderpass = device.createRenderPass(renderpass_create_info).value;
}

void Renderer::InitFramebuffers()
{
    for (const auto& swapchain_imageview : graphics_ptr->GetSwapchainImageViews())
    {
        std::vector<vk::ImageView> attachments;
        attachments.emplace_back(visibilityImageView);
        attachments.emplace_back(depthImageView);
        attachments.emplace_back(swapchain_imageview);

        vk::FramebufferCreateInfo framebuffer_createInfo;
        framebuffer_createInfo.renderPass = renderpass;
        framebuffer_createInfo.setAttachments(attachments);
        framebuffer_createInfo.width = graphics_ptr->GetSwapchainCreateInfo().imageExtent.width;
        framebuffer_createInfo.height = graphics_ptr->GetSwapchainCreateInfo().imageExtent.height;
        framebuffer_createInfo.layers = 1;

        framebuffers.emplace_back(device.createFramebuffer(framebuffer_createInfo).value);
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

    {   // submitFinishTimelineSemaphore
        vk::SemaphoreTypeCreateInfo semaphore_type_info;
        semaphore_type_info.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphore_type_info.initialValue = 0;

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_info;

        submitFinishTimelineSemaphore = device.createSemaphore(semaphore_create_info).value;
    }

    {   // hostWriteFinishTimelineSemaphore
        vk::SemaphoreTypeCreateInfo semaphore_type_info;
        semaphore_type_info.semaphoreType = vk::SemaphoreType::eTimeline;
        semaphore_type_info.initialValue = 0;

        vk::SemaphoreCreateInfo semaphore_create_info;
        semaphore_create_info.pNext = &semaphore_type_info;

        hostWriteFinishTimelineSemaphore = device.createSemaphore(semaphore_create_info).value;
    }
    vk::SemaphoreCreateInfo semaphore_create_info;
}

void Renderer::InitCommandBuffers()
{
    vk::CommandPoolCreateInfo command_pool_create_info;
    command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eTransient | vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    command_pool_create_info.queueFamilyIndex = graphicsQueue.second;

    commandPool = device.createCommandPool(command_pool_create_info).value;

    vk::CommandBufferAllocateInfo command_buffer_alloc_info;
    command_buffer_alloc_info.commandPool = commandPool;
    command_buffer_alloc_info.level = vk::CommandBufferLevel::ePrimary;
    command_buffer_alloc_info.commandBufferCount = 3;

    auto command_buffers = device.allocateCommandBuffers(command_buffer_alloc_info).value;
    commandBuffers[0] = command_buffers[0];
    commandBuffers[1] = command_buffers[1];
    commandBuffers[2] = command_buffers[2];
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

void Renderer::InitFullscreenPipeline()
{
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
        descriptor_sets_layouts.emplace_back(primitivesInstanceDescriptorSetLayout);
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

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

        ShadersSpecs shaders_specs {"Texture-Pass Shaders", shadersDefinitionStringPairs};
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

void Renderer::InitTLASes()
{
    // Get required sizes
    vk::AccelerationStructureBuildSizesInfoKHR build_size_info;
    {
        vk::AccelerationStructureGeometryKHR instances;
        instances.geometryType = vk::GeometryTypeKHR::eInstances;
        instances.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;

        vk::AccelerationStructureBuildGeometryInfoKHR geometry_info;
        geometry_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        geometry_info.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        geometry_info.geometryCount = 1;
        geometry_info.pGeometries = &instances;

        build_size_info = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                                       geometry_info,
                                                                       graphics_ptr->GetMaxInstancesCount());
    }
    TLASesHalfSize = build_size_info.accelerationStructureSize;
    TLASesHalfSize += (TLASesHalfSize % 256 != 0) ? 256 - TLASesHalfSize % 256 : 0;

    // Create buffer for acceleration structures
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = TLASesHalfSize * 2;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo allocation_create_info;
        allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto create_buffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
        assert(create_buffer_result.result == vk::Result::eSuccess);
        TLASesBuffer = create_buffer_result.value.first;
        TLASesAllocation = create_buffer_result.value.second;
    }

    // Create TLASes
    for(size_t i = 0; i != 2; ++i) {
        vk::AccelerationStructureCreateInfoKHR TLAS_create_info;
        TLAS_create_info.buffer = TLASesBuffer;
        TLAS_create_info.size = build_size_info.accelerationStructureSize;
        TLAS_create_info.offset = i * TLASesHalfSize;
        TLAS_create_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        auto TLAS_create_result = device.createAccelerationStructureKHR(TLAS_create_info);
        assert(TLAS_create_result.result == vk::Result::eSuccess);
        TLASesHandles[i] = TLAS_create_result.value;
        TLASesDeviceAddresses[i] = device.getAccelerationStructureAddressKHR({TLASesHandles[i]});
    }

    // Create scratch build buffer
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = build_size_info.buildScratchSize;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo allocation_create_info;
        allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        TLASbuildScratchBuffer = createBuffer_result.value.first;
        TLASbuildScratchAllocation = createBuffer_result.value.second;
    }


}

void Renderer::DrawFrame(ViewportFrustum viewport,
                         std::vector<ModelMatrices>&& matrices,
                         std::vector<DrawInfo>&& draw_infos)
{
    ++frameCount;

    size_t buffer_index = frameCount % 2;
    size_t commandBuffer_index = frameCount % 3;

    graphics_ptr->GetDynamicMeshes()->SwapDescriptorSet(frameCount);
    std::vector<PrimitiveInstanceParameters> primitive_instance_parameters = CreatePrimitivesInstanceParameters(draw_infos);
    std::vector<vk::AccelerationStructureInstanceKHR> TLAS_instances = CreateTLASinstances(draw_infos, matrices, buffer_index);

    vk::CommandBuffer command_buffer = commandBuffers[commandBuffer_index];
    command_buffer.reset();

    FrustumCulling frustum_culling;
    frustum_culling.SetFrustumPlanes(viewport.GetWorldSpacePlanesOfFrustum());

    uint32_t swapchain_index = device.acquireNextImageKHR(graphics_ptr->GetSwapchain(),
                                                          0,
                                                          presentImageAvailableSemaphores[commandBuffer_index]).value;

    RecordCommandBuffer(command_buffer, uint32_t(buffer_index), swapchain_index, uint32_t(TLAS_instances.size()), draw_infos, frustum_culling);

    // Submit but hold until write host
    vk::SubmitInfo submit_info;
    vk::TimelineSemaphoreSubmitInfo timeline_semaphore_info;
    submit_info.pNext = &timeline_semaphore_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;
    /// wait for
    std::array<vk::Semaphore, 2> wait_semaphores = {hostWriteFinishTimelineSemaphore, presentImageAvailableSemaphores[commandBuffer_index]};
    std::array<vk::PipelineStageFlags, 2> wait_pipeline_stages = {vk::PipelineStageFlagBits::eTopOfPipe ,vk::PipelineStageFlagBits::eColorAttachmentOutput};
    submit_info.setWaitSemaphores(wait_semaphores);
    submit_info.setWaitDstStageMask(wait_pipeline_stages);
    std::array<uint64_t, 2> wait_semaphores_values = {frameCount, 0};
    timeline_semaphore_info.setWaitSemaphoreValues(wait_semaphores_values);
    /// signal
    std::array<vk::Semaphore, 2> signal_semaphores = {submitFinishTimelineSemaphore, readyForPresentSemaphores[commandBuffer_index]};
    submit_info.setSignalSemaphores(signal_semaphores);
    std::array<uint64_t, 2> signal_semaphores_values = {frameCount, 0};
    timeline_semaphore_info.setSignalSemaphoreValues(signal_semaphores_values);

    std::vector<vk::SubmitInfo> submit_infos;
    submit_infos.emplace_back(submit_info);
    graphicsQueue.first.submit(submit_infos);

    //
    // Wait! Wait for write buffers (-2)
    {
        uint64_t wait_value = uint64_t( std::max(int64_t(frameCount - 2), int64_t(0)) );

        vk::SemaphoreWaitInfo host_wait_info;
        host_wait_info.semaphoreCount = 1;
        host_wait_info.pSemaphores = &submitFinishTimelineSemaphore;
        host_wait_info.pValues = &wait_value;

        device.waitSemaphores(host_wait_info, uint64_t(-1));
    }

    graphics_ptr->WriteCameraMarticesBuffers(viewport,
                                             matrices,
                                             draw_infos,
                                             buffer_index);

    {
        memcpy((std::byte*)(primitivesInstanceAllocInfo.pMappedData) + buffer_index * primitivesInstanceBufferHalfsize,
               primitive_instance_parameters.data(),
               primitive_instance_parameters.size() * sizeof(PrimitiveInstanceParameters));
        vma_allocator.flushAllocation(primitivesInstanceAllocation,
                                      buffer_index * primitivesInstanceBufferHalfsize,
                                      primitive_instance_parameters.size() * sizeof(PrimitiveInstanceParameters));
    }

    {
        memcpy((std::byte*)(TLASesInstancesAllocInfo.pMappedData) + buffer_index * TLASesInstancesHalfSize,
               TLAS_instances.data(),
               TLAS_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));

        vma_allocator.flushAllocation(TLASesInstancesAllocation,
                                      buffer_index * TLASesInstancesHalfSize,
                                      TLAS_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));
    }

    {
        std::array<std::array<glm::vec4, 4>, 2> vertex_data = {viewport.GetFullscreenpassTrianglePos(),
                                                               viewport.GetFullscreenpassTriangleNormals()};

        memcpy((std::byte*)(fullscreenAllocInfo.pMappedData) + buffer_index * fullscreenBufferHalfsize,
               vertex_data.data(),
               fullscreenBufferHalfsize);
        vma_allocator.flushAllocation(fullscreenAllocation,
                                      buffer_index * fullscreenBufferHalfsize,
                                      fullscreenBufferHalfsize);
    }

    //
    // Signal host write finish semaphore
    {
        vk::SemaphoreSignalInfo host_signal_info;
        host_signal_info.semaphore = hostWriteFinishTimelineSemaphore;
        host_signal_info.value = uint64_t(frameCount);

        device.signalSemaphore(host_signal_info);
    }

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

void Renderer::RecordCommandBuffer(vk::CommandBuffer command_buffer,
                                   uint32_t buffer_index,
                                   uint32_t swapchain_index,
                                   uint32_t TLAS_primitive_count,
                                   const std::vector<DrawInfo>& draw_infos,
                                   const FrustumCulling& frustum_culling)
{
    command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    // Update dynamic meshes
    std::vector<DrawInfo> dynamic_meshes_draw_infos;
    for(const DrawInfo& this_draw_info : draw_infos) {
        if (this_draw_info.isSkin || this_draw_info.hasMorphTargets)
            dynamic_meshes_draw_infos.emplace_back(this_draw_info);
    }
    graphics_ptr->GetDynamicMeshes()->RecordTransformations(command_buffer, dynamic_meshes_draw_infos);

    // Update TLAS
    vk::AccelerationStructureGeometryKHR geometry_instance;
    geometry_instance.geometryType = vk::GeometryTypeKHR::eInstances;
    geometry_instance.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
    geometry_instance.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry_instance.geometry.instances.data = device.getBufferAddress(TLASesInstancesBuffer) + buffer_index * TLASesInstancesHalfSize;

    vk::AccelerationStructureBuildGeometryInfoKHR geometry_info;
    geometry_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    geometry_info.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    geometry_info.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geometry_info.dstAccelerationStructure = TLASesHandles[buffer_index];
    geometry_info.geometryCount = 1;
    geometry_info.pGeometries = &geometry_instance;
    geometry_info.scratchData = device.getBufferAddress(TLASbuildScratchBuffer);

    vk::AccelerationStructureBuildRangeInfoKHR build_range = {};
    build_range.primitiveCount = TLAS_primitive_count;

    vk::AccelerationStructureBuildRangeInfoKHR* indirection = &build_range;
    command_buffer.buildAccelerationStructuresKHR(1, &geometry_info, &indirection);

    // -- Barrier --
    {
        vk::BufferMemoryBarrier this_memory_barrier;
        this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
        this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        this_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        this_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        this_memory_barrier.buffer = TLASesBuffer;
        this_memory_barrier.offset = buffer_index * TLASesHalfSize;
        this_memory_barrier.size = TLASesHalfSize;

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::PipelineStageFlagBits::eFragmentShader | vk::PipelineStageFlagBits::eRayTracingShaderKHR,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       this_memory_barrier,
                                       {});
    }


    // Render begin
    vk::RenderPassBeginInfo render_pass_begin_info;
    vk::ClearValue clear_values[3];
    render_pass_begin_info.renderPass = renderpass;
    render_pass_begin_info.framebuffer = framebuffers[swapchain_index];
    render_pass_begin_info.renderArea.offset = vk::Offset2D(0, 0);
    render_pass_begin_info.renderArea.extent = graphics_ptr->GetSwapchainCreateInfo().imageExtent;

    std::array<uint32_t, 4> visibility_clear = {0, 0, 0, 0};
    clear_values[0].color.uint32 = visibility_clear;
    clear_values[1].depthStencil.depth = 1.f;
    std::array<float, 4> color_clear = {0.04f, 0.08f, 0.f, 1.f};
    clear_values[2].color.float32 = color_clear;

    render_pass_begin_info.clearValueCount = 3;
    render_pass_begin_info.pClearValues = clear_values;

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    // Visibility pass
    for (const DrawInfo& this_draw: draw_infos) {
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
            descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(buffer_index));
            descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(buffer_index));
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
                offsets.emplace_back(this_draw_primitive_info.dynamicPrimitiveInfo.positionByteOffset + buffer_index * this_draw_primitive_info.dynamicBufferHalfSize);
                buffers.emplace_back(this_draw_primitive_info.dynamicBuffer);
            } else {
                offsets.emplace_back(this_draw_primitive_info.primitiveInfo.positionByteOffset);
                buffers.emplace_back(static_primitives_buffer);
            }

            // Color texcoords if material is masked
            if (this_material.masked) {
                if (this_draw_primitive_info.dynamicPrimitiveInfo.texcoordsByteOffset != -1) {
                    offsets.emplace_back(this_draw_primitive_info.dynamicPrimitiveInfo.texcoordsByteOffset + this_material.color_texcooord * sizeof(glm::vec2)
                                        + buffer_index * this_draw_primitive_info.dynamicBufferHalfSize );
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

    // Texture pass
    {
        command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, fullscreenPipeline);

        std::vector<vk::DescriptorSet> descriptor_sets;
        descriptor_sets.emplace_back(graphics_ptr->GetCameraDescriptionSet(buffer_index));
        descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(buffer_index));
        descriptor_sets.emplace_back(graphics_ptr->GetDynamicMeshes()->GetDescriptorSet());
        descriptor_sets.emplace_back(graphics_ptr->GetMaterialsOfPrimitives()->GetDescriptorSet());
        descriptor_sets.emplace_back(primitivesInstanceDescriptorSets[buffer_index]);

        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                          fullscreenPipelineLayout,
                                          0,
                                          descriptor_sets,
                                          {});

        std::vector<vk::Buffer> buffers;
        std::vector<vk::DeviceSize> offsets;

        buffers.emplace_back(fullscreenBuffer);
        offsets.emplace_back(buffer_index * fullscreenBufferHalfsize);

        command_buffer.bindVertexBuffers(0, buffers, offsets);

        command_buffer.draw(4, 1, 0, 0);
    }

    command_buffer.endRenderPass();

    command_buffer.end();
}

std::vector<Renderer::PrimitiveInstanceParameters> Renderer::CreatePrimitivesInstanceParameters(std::vector<DrawInfo>& draw_infos) const
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

    for (DrawInfo& this_draw_info : draw_infos) {
        this_draw_info.primitivesInstanceOffset = return_vector.size();

        std::vector<PrimitiveInfo> primitives_info;
        std::vector<DynamicMeshInfo::DynamicPrimitiveInfo> dynamic_primitives_info;
        uint32_t descriptor_index = 0;
        if (this_draw_info.dynamicMeshIndex != -1) {
            descriptor_index = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex).descriptorIndex;
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
                if (primitives_info[i].texcoordsByteOffset != -1) {
                    this_primitiveInstanceParameters.colorStepMultiplier = 1;
                    this_primitiveInstanceParameters.colorOffset = primitives_info[i].texcoordsByteOffset / sizeof(glm::vec4);
                    this_primitiveInstanceParameters.colorDescriptorIndex = 0;
                } else {
                    this_primitiveInstanceParameters.colorStepMultiplier = 0;
                    this_primitiveInstanceParameters.colorOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().colorByteOffset / sizeof(glm::vec2);
                    this_primitiveInstanceParameters.colorDescriptorIndex = 0;
                }
            }

            return_vector.emplace_back(this_primitiveInstanceParameters);
        }
    }

    return return_vector;
}

std::vector<vk::AccelerationStructureInstanceKHR> Renderer::CreateTLASinstances(const std::vector<DrawInfo>& draw_infos,
                                                                                const std::vector<ModelMatrices>& matrices,
                                                                                uint32_t buffer_index) const
{
    std::vector<vk::AccelerationStructureInstanceKHR> return_vector;
    for (const auto& this_draw_info : draw_infos) {
        if (this_draw_info.dynamicMeshIndex != -1) {
            const MeshInfo& mesh_info = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw_info.meshIndex);
            const DynamicMeshInfo& dynamic_mesh_info = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex);
            if (dynamic_mesh_info.hasDynamicBLAS) {
                vk::AccelerationStructureInstanceKHR instance;
                const glm::mat4& matrix = matrices[this_draw_info.matricesOffset].positionMatrix;
                instance.transform = { matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                                       matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                                       matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] };
                instance.instanceCustomIndex = this_draw_info.primitivesInstanceOffset;
                instance.mask = 0xFF;
                instance.instanceShaderBindingTableRecordOffset = 0;
                instance.flags = mesh_info.meshBLAS.disableFaceCulling ? uint8_t(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable) : 0;
                instance.accelerationStructureReference = dynamic_mesh_info.BLASesDeviceAddresses[buffer_index];

                return_vector.emplace_back(instance);
            }
        } else {
            const MeshInfo& mesh_info = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw_info.meshIndex);
            if (mesh_info.meshBLAS.hasBLAS) {
                vk::AccelerationStructureInstanceKHR instance;
                const glm::mat4& matrix = matrices[this_draw_info.matricesOffset].positionMatrix;
                instance.transform = { matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                                       matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                                       matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] };
                instance.instanceCustomIndex = this_draw_info.primitivesInstanceOffset;
                instance.mask = 0xFF;
                instance.instanceShaderBindingTableRecordOffset = 0;
                instance.flags = mesh_info.meshBLAS.disableFaceCulling ? uint8_t(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable) : 0;
                instance.accelerationStructureReference = mesh_info.meshBLAS.deviceAddress;

                return_vector.emplace_back(instance);
            }
        }
    }

    return return_vector;
}
