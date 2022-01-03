#include "Graphics/Graphics.h"
#include "Geometry/FrustumCulling.h"
#include "Engine.h"

#include <utility>
#include <cassert>
#include <array>

Graphics::Graphics(Engine* in_engine_ptr, configuru::Config& in_cfgFile, vk::Device in_device, vma::Allocator in_vma_allocator)
    :engine_ptr(in_engine_ptr),
     cfgFile(in_cfgFile),
     device(in_device),
     vma_allocator(in_vma_allocator)
{
    graphicsQueue = engine_ptr->GetQueuesList().graphicsQueues[0];

    printf("Initializing camera buffers\n");
    InitBuffers();
    printf("Initializing camera descriptor set\n");
    InitDescriptors();
    printf("Initializing GPU images (z-buffers etc)\n");
    InitImages();
    printf("Initializing renderpasses\n");
    InitRenderpasses();
    printf("Initializing framebuffers\n");
    InitFramebuffers();
    printf("Initializing semaphores\n");
    InitSemaphoresAndFences();
    printf("Initializing command buffers\n");
    InitCommandBuffers();
    printf("Initializing pipelines factory\n");
    InitPipelinesFactory();
    printf("Initializing shaders sets families cache\n");
    InitShadersSetsFamiliesCache();
    printf("Initializing meshes tree\n");
    InitMeshesTree();
    printf("Initializing dynamic meshes\n");
    InitDynamicMeshes();
    printf("Initializing graphics oriented components\n");
    InitGraphicsComponents();
}

Graphics::~Graphics()
{
    device.waitIdle();

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

    device.destroy(descriptorPool);
    device.destroy(cameraDescriptorSetLayout);
    device.destroy(matricesDescriptorSetLayout);

    vma_allocator.destroyBuffer(cameraBuffer, cameraAllocation);
    vma_allocator.destroyBuffer(matricesBuffer, matricesAllocation);

    dynamicMeshes_uptr.reset();
    meshesOfNodes_uptr.reset();
    skinsOfMeshes_uptr.reset();
    materialsOfPrimitives_uptr.reset();
    texturesOfMaterials_uptr.reset();
    shadersSetsFamiliesCache_uptr.reset();
    pipelinesFactory_uptr.reset();
    primitivesOfMeshes_uptr.reset();
}

void Graphics::DrawFrame()
{
    ++frameCount;

    size_t buffer_index = frameCount % 2;
    size_t commandBuffer_index = frameCount % 3;

    std::vector<glm::mat4> matrices;
    std::vector<DrawInfo> draw_infos;
    modelDrawComp_uptr->AddDrawInfos(matrices, draw_infos);

    dynamicMeshes_uptr->SwapDescriptorSet(frameCount);

    ViewportFrustum culling_viewport = cameraComp_uptr->GetBindedCameraEntity()->cullingViewportFrustum;
    FrustumCulling frustum_culling;
    frustum_culling.SetFrustumPlanes(culling_viewport.GetWorldSpacePlanesOfFrustum());

    //
    // Wait! For command buffer reset (-3)
    {
        uint64_t wait_value = uint64_t( std::max(int64_t(frameCount - 3), int64_t(0)) );

        vk::SemaphoreWaitInfo host_wait_info;
        host_wait_info.semaphoreCount = 1;
        host_wait_info.pSemaphores = &submitFinishTimelineSemaphore;
        host_wait_info.pValues = &wait_value;

        device.waitSemaphores(host_wait_info, uint64_t(-1));
    }

    vk::CommandBuffer command_buffer = commandBuffers[commandBuffer_index];
    command_buffer.reset();

    uint32_t swapchainIndex = device.acquireNextImageKHR(engine_ptr->GetSwapchain(),
                                                         0,
                                                         presentImageAvailableSemaphores[commandBuffer_index]).value;

    RecordCommandBuffer(command_buffer, uint32_t(buffer_index), swapchainIndex, draw_infos, frustum_culling);

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

    // Present but hold
    vk::PresentInfoKHR present_info;
    vk::SwapchainKHR swapchain = engine_ptr->GetSwapchain();
    present_info.waitSemaphoreCount = 1;
    present_info.pWaitSemaphores = &readyForPresentSemaphores[commandBuffer_index];
    present_info.swapchainCount = 1;
    present_info.pSwapchains = &swapchain;
    present_info.pImageIndices = &swapchainIndex;

    graphicsQueue.first.presentKHR(present_info);

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
    
    ViewportFrustum camera_viewport = cameraComp_uptr->GetBindedCameraEntity()->cameraViewportFrustum;

    // Update camera matrix
    {
        glm::mat4x4 view_projection_matrices[2] = {camera_viewport.GetViewMatrix(), camera_viewport.GetPerspectiveMatrix()};

        memcpy((std::byte*)(cameraAllocInfo.pMappedData) + buffer_index * 2 * sizeof(glm::mat4),
               view_projection_matrices,
               2 * sizeof(glm::mat4));
        vma_allocator.flushAllocation(cameraAllocation, buffer_index * sizeof(glm::mat4), sizeof(glm::mat4));
    }

    // Update matrices
    {
        memcpy((std::byte *) (matricesAllocInfo.pMappedData) + buffer_index * maxInstances * sizeof(glm::mat4),
               matrices.data(),
               matrices.size() * sizeof(glm::mat4));

        vma_allocator.flushAllocation(matricesAllocation, buffer_index * maxInstances * sizeof(glm::mat4), matrices.size() * sizeof(glm::mat4));
    }

    //
    // Signal host write finish semaphore
    {
        vk::SemaphoreSignalInfo host_signal_info;
        host_signal_info.semaphore = hostWriteFinishTimelineSemaphore;
        host_signal_info.value = uint64_t(frameCount);

        device.signalSemaphore(host_signal_info);
    }

    dynamicMeshes_uptr->CompleteRemovesSafe();
}

void Graphics::RecordCommandBuffer(vk::CommandBuffer command_buffer,
                                   uint32_t buffer_index,
                                   uint32_t swapchain_index,
                                   std::vector<DrawInfo>& draw_infos,
                                   const FrustumCulling& frustum_culling)
{
    command_buffer.begin(vk::CommandBufferBeginInfo(vk::CommandBufferUsageFlagBits::eOneTimeSubmit));

    std::vector<DrawInfo> dynamic_meshes_draw_infos;
    for(const DrawInfo& this_draw_info : draw_infos) {
        if (this_draw_info.isSkin || this_draw_info.hasMorphTargets)
            dynamic_meshes_draw_infos.emplace_back(this_draw_info);
    }
    dynamicMeshes_uptr->RecordTransformations(command_buffer, dynamic_meshes_draw_infos);

    vk::RenderPassBeginInfo render_pass_begin_info;
    vk::ClearValue clear_values[2];
    render_pass_begin_info.renderPass = renderpass;
    render_pass_begin_info.framebuffer = framebuffers[swapchain_index];
    render_pass_begin_info.renderArea.offset = vk::Offset2D(0, 0);
    render_pass_begin_info.renderArea.extent = engine_ptr->GetSwapchainCreateInfo().imageExtent;

    std::array<float, 4> color_clear = {0.04f, 0.08f, 0.f, 1.f};
    clear_values[0].color.float32 = color_clear;
    clear_values[1].depthStencil.depth = 1.f;
    render_pass_begin_info.clearValueCount = 2;
    render_pass_begin_info.pClearValues = clear_values;

    command_buffer.beginRenderPass(render_pass_begin_info, vk::SubpassContents::eInline);

    for (const DrawInfo& this_draw: draw_infos) {
        std::vector<std::tuple<size_t, PrimitiveInfo, DynamicPrimitiveInfo>> info_tuples;

        if (this_draw.dynamicMeshIndex != -1) {
            const auto &dynamic_primitives_infos = dynamicMeshes_uptr->GetDynamicPrimitivesInfo(this_draw.dynamicMeshIndex);
            for (const auto& this_dynamic_primitive_info: dynamic_primitives_infos) {
                const PrimitiveInfo &this_primitive_info = primitivesOfMeshes_uptr->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);
                info_tuples.emplace_back(this_dynamic_primitive_info.primitiveIndex, this_primitive_info, this_dynamic_primitive_info);
            }
        } else {
            for (size_t primitive_index : meshesOfNodes_uptr->GetMeshInfo(this_draw.meshIndex).primitivesIndex) {
                const PrimitiveInfo& primitive_info = primitivesOfMeshes_uptr->GetPrimitiveInfo(primitive_index);
                info_tuples.emplace_back(primitive_index, primitive_info, DynamicPrimitiveInfo());
            }
        }

        for (const auto& this_info_tuple : info_tuples) {
            vk::Pipeline pipeline = primitivesPipelines[std::get<0>(this_info_tuple)];
            command_buffer.bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline);

            vk::PipelineLayout pipeline_layout = primitivesPipelineLayouts[std::get<0>(this_info_tuple)];
            std::vector<vk::DescriptorSet> descriptor_sets;
            descriptor_sets.emplace_back(cameraDescriptorSets[buffer_index]);
            descriptor_sets.emplace_back(matricesDescriptorSets[buffer_index]);
            descriptor_sets.emplace_back(materialsOfPrimitives_uptr->GetDescriptorSet());
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eGraphics,
                                              pipeline_layout,
                                              0,
                                              descriptor_sets,
                                              {});

            std::array<uint32_t, 1> data_vertex = {uint32_t(this_draw.matricesOffset)};
            command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eVertex, 0, 4, data_vertex.data());

            std::array<uint32_t, 1> data_frag = {uint32_t(std::get<1>(this_info_tuple).material)};
            command_buffer.pushConstants(pipeline_layout, vk::ShaderStageFlagBits::eFragment, 4, 4, data_frag.data());

            vk::Buffer staticBuffer = primitivesOfMeshes_uptr->GetVerticesBuffer();
            std::vector<vk::Buffer> buffers;
            std::vector<vk::DeviceSize> offsets;

            // Position
            if (std::get<2>(this_info_tuple).positionOffset != -1) {
                offsets.emplace_back(std::get<2>(this_info_tuple).positionOffset + buffer_index * std::get<2>(this_info_tuple).halfSize);
                buffers.emplace_back(std::get<2>(this_info_tuple).buffer);
            } else {
                offsets.emplace_back(std::get<1>(this_info_tuple).positionOffset);
                buffers.emplace_back(staticBuffer);
            }

            // Normal
            if (std::get<1>(this_info_tuple).normalOffset != -1) {
                if (std::get<2>(this_info_tuple).normalOffset != -1) {
                    offsets.emplace_back(std::get<2>(this_info_tuple).normalOffset + buffer_index * std::get<2>(this_info_tuple).halfSize);
                    buffers.emplace_back(std::get<2>(this_info_tuple).buffer);
                } else {
                    offsets.emplace_back(std::get<1>(this_info_tuple).normalOffset);
                    buffers.emplace_back(staticBuffer);
                }
            }

            // Tangent
            if (std::get<1>(this_info_tuple).tangentOffset != -1) {
                if (std::get<2>(this_info_tuple).tangentOffset != -1) {
                    offsets.emplace_back(std::get<2>(this_info_tuple).tangentOffset + buffer_index * std::get<2>(this_info_tuple).halfSize);
                    buffers.emplace_back(std::get<2>(this_info_tuple).buffer);
                } else {
                    offsets.emplace_back(std::get<1>(this_info_tuple).tangentOffset);
                    buffers.emplace_back(staticBuffer);
                }
            }

            // Texcoords
            if (std::get<1>(this_info_tuple).texcoordsOffset != -1) {
                if (std::get<2>(this_info_tuple).texcoordsOffset != -1) {
                    offsets.emplace_back(std::get<2>(this_info_tuple).texcoordsOffset + buffer_index * std::get<2>(this_info_tuple).halfSize);
                    buffers.emplace_back(std::get<2>(this_info_tuple).buffer);
                } else {
                    offsets.emplace_back(std::get<1>(this_info_tuple).texcoordsOffset);
                    buffers.emplace_back(staticBuffer);
                }
            }

            // Color
            if (std::get<1>(this_info_tuple).colorOffset != -1) {
                if (std::get<2>(this_info_tuple).colorOffset != -1) {
                    offsets.emplace_back(std::get<2>(this_info_tuple).colorOffset + buffer_index * std::get<2>(this_info_tuple).halfSize);
                    buffers.emplace_back(std::get<2>(this_info_tuple).buffer);
                } else {
                    offsets.emplace_back(std::get<1>(this_info_tuple).colorOffset);
                    buffers.emplace_back(staticBuffer);
                }
            }

            command_buffer.bindVertexBuffers(0, buffers, offsets);

            command_buffer.bindIndexBuffer(primitivesOfMeshes_uptr->GetIndicesBuffer(),
                                           std::get<1>(this_info_tuple).indicesOffset,
                                           vk::IndexType::eUint32);

            command_buffer.drawIndexed(uint32_t(std::get<1>(this_info_tuple).indicesCount), 1, 0, 0, 0);
        }
    }

    command_buffer.endRenderPass();

    command_buffer.end();
}

void Graphics::InitBuffers()
{
    // Create camera buffer
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof(glm::mat4) * 4;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              cameraAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        cameraBuffer = createBuffer_result.value.first;
        cameraAllocation = createBuffer_result.value.second;
    }

    // Create matrices buffer
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof(glm::mat4) * maxInstances * 2;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              matricesAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        matricesBuffer = createBuffer_result.value.first;
        matricesAllocation = createBuffer_result.value.second;
    }
}

void Graphics::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eUniformBuffer, 2);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 2);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 4,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create camera layout
        vk::DescriptorSetLayoutBinding buffer_binding;
        buffer_binding.binding = 0;
        buffer_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        buffer_binding.descriptorCount = 1;
        buffer_binding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, 1, &buffer_binding);
        cameraDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Create matrices layout
        vk::DescriptorSetLayoutBinding buffer_binding;
        buffer_binding.binding = 0;
        buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
        buffer_binding.descriptorCount = 1;
        buffer_binding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, 1, &buffer_binding);
        matricesDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Allocate descriptor sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        cameraDescriptorSets[0] = descriptor_sets[0];
        cameraDescriptorSets[1] = descriptor_sets[1];
        matricesDescriptorSets[0] = descriptor_sets[2];
        matricesDescriptorSets[1] = descriptor_sets[3];
    }

    {   // Writing descriptor set
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;

        {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = cameraBuffer;
            descriptor_buffer_info_uptr->offset = 0;
            descriptor_buffer_info_uptr->range  = 2 * sizeof(glm::mat4);

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = cameraDescriptorSets[0];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eUniformBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }
        {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = cameraBuffer;
            descriptor_buffer_info_uptr->offset = 2 * sizeof(glm::mat4);
            descriptor_buffer_info_uptr->range  = 2 * sizeof(glm::mat4);

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = cameraDescriptorSets[1];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eUniformBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }
        {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = matricesBuffer;
            descriptor_buffer_info_uptr->offset = 0;
            descriptor_buffer_info_uptr->range  = sizeof(glm::mat4) * maxInstances;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = matricesDescriptorSets[0];
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
            descriptor_buffer_info_uptr->buffer = matricesBuffer;
            descriptor_buffer_info_uptr->offset = sizeof(glm::mat4) * maxInstances;
            descriptor_buffer_info_uptr->range  = sizeof(glm::mat4) * maxInstances;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = matricesDescriptorSets[1];
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
}

void Graphics::InitImages()
{
    // z-buffer buffer
    {
        depthImageCreateInfo.imageType = vk::ImageType::e2D;
        depthImageCreateInfo.format = vk::Format::eD32Sfloat;
        depthImageCreateInfo.extent.width = engine_ptr->GetSwapchainCreateInfo().imageExtent.width;
        depthImageCreateInfo.extent.height = engine_ptr->GetSwapchainCreateInfo().imageExtent.height;
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
}

void Graphics::InitRenderpasses()
{
    // Attachments
    vk::AttachmentDescription attachment_descriptions[2];

    attachment_descriptions[0].format = engine_ptr->GetSwapchainCreateInfo().imageFormat;
    attachment_descriptions[0].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachment_descriptions[0].stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[0].initialLayout = vk::ImageLayout::eUndefined;
    attachment_descriptions[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    attachment_descriptions[1].format = depthImageCreateInfo.format;
    attachment_descriptions[1].samples = vk::SampleCountFlagBits::e1;
    attachment_descriptions[1].loadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[1].storeOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[1].stencilLoadOp = vk::AttachmentLoadOp::eClear;
    attachment_descriptions[1].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachment_descriptions[1].initialLayout = vk::ImageLayout::eUndefined;
    attachment_descriptions[1].finalLayout = vk::ImageLayout::eDepthStencilAttachmentOptimal;

    // Subpass
    vk::AttachmentReference colorAttach_ref;
    colorAttach_ref.layout = vk::ImageLayout::eColorAttachmentOptimal;
    colorAttach_ref.attachment = 0;

    vk::AttachmentReference depthAttach_ref;
    depthAttach_ref.layout = vk::ImageLayout::eDepthStencilAttachmentOptimal;
    depthAttach_ref.attachment = 1;

    vk::SubpassDescription subpass_description;
    subpass_description.pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpass_description.colorAttachmentCount = 1;
    subpass_description.pColorAttachments = &colorAttach_ref;
    subpass_description.pDepthStencilAttachment = &depthAttach_ref;

    vk::SubpassDependency subpass_dependency;
    subpass_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    subpass_dependency.dstSubpass = 0;
    subpass_dependency.srcStageMask = vk::PipelineStageFlagBits::eLateFragmentTests;
    subpass_dependency.dstStageMask = vk::PipelineStageFlagBits::eEarlyFragmentTests;
    subpass_dependency.srcAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;
    subpass_dependency.dstAccessMask = vk::AccessFlagBits::eDepthStencilAttachmentWrite | vk::AccessFlagBits::eDepthStencilAttachmentRead;

    // Renderpass
    vk::RenderPassCreateInfo renderpass_create_info;
    renderpass_create_info.attachmentCount = 2;
    renderpass_create_info.pAttachments = attachment_descriptions;
    renderpass_create_info.subpassCount = 1;
    renderpass_create_info.pSubpasses = &subpass_description;
    renderpass_create_info.dependencyCount = 1;
    renderpass_create_info.pDependencies = &subpass_dependency;

    renderpass = device.createRenderPass(renderpass_create_info).value;
}

void Graphics::InitFramebuffers()
{
    for (const auto& swapchain_imageview : engine_ptr->GetSwapchainImageViews())
    {
        std::vector<vk::ImageView> attachments;
        attachments.emplace_back(swapchain_imageview);
        attachments.emplace_back(depthImageView);

        vk::FramebufferCreateInfo framebuffer_createInfo;
        framebuffer_createInfo.renderPass = renderpass;
        framebuffer_createInfo.setAttachments(attachments);
        framebuffer_createInfo.width = engine_ptr->GetSwapchainCreateInfo().imageExtent.width;
        framebuffer_createInfo.height = engine_ptr->GetSwapchainCreateInfo().imageExtent.height;
        framebuffer_createInfo.layers = 1;

        framebuffers.emplace_back(device.createFramebuffer(framebuffer_createInfo).value);
    }
}


void Graphics::InitSemaphoresAndFences()
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

void Graphics::InitCommandBuffers()
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

void Graphics::InitPipelinesFactory()
{
    pipelinesFactory_uptr = std::make_unique<PipelinesFactory>(device);
}

void Graphics::InitShadersSetsFamiliesCache()
{
    shadersSetsFamiliesCache_uptr = std::make_unique<ShadersSetsFamiliesCache>(device, "shaders");

    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Texture-Pass Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "generalShader_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "generalShader_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Dynamic Mesh Evaluation Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "dynamicMeshShader_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
}

void Graphics::InitMeshesTree()
{
    animationsDataOfNodes_uptr = std::make_unique<AnimationsDataOfNodes>();

    texturesOfMaterials_uptr = std::make_unique<TexturesOfMaterials>(device, vma_allocator, engine_ptr->GetQueuesList().graphicsQueues[0]);

    materialsOfPrimitives_uptr = std::make_unique<MaterialsOfPrimitives>(texturesOfMaterials_uptr.get(), device, vma_allocator);                                                               //needs flash

    primitivesOfMeshes_uptr = std::make_unique<PrimitivesOfMeshes>(materialsOfPrimitives_uptr.get(), device, vma_allocator);

    skinsOfMeshes_uptr = std::make_unique<SkinsOfMeshes>(device, vma_allocator);

    meshesOfNodes_uptr = std::make_unique<MeshesOfNodes>(primitivesOfMeshes_uptr.get());

}

void Graphics::InitDynamicMeshes()
{
    dynamicMeshes_uptr = std::make_unique<DynamicMeshes>(this, device, vma_allocator, 127);
}

void Graphics::InitGraphicsComponents()
{
    {
        animationActorComp_uptr = std::make_unique<AnimationActorComp>(engine_ptr->GetECSwrapperPtr(),
                                                                       animationsDataOfNodes_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(animationActorComp_uptr.get());
    }
    {
        uint32_t width = engine_ptr->GetSwapchainCreateInfo().imageExtent.width;
        uint32_t height = engine_ptr->GetSwapchainCreateInfo().imageExtent.height;

        cameraComp_uptr = std::make_unique<CameraComp>(engine_ptr->GetECSwrapperPtr(),
                                                       glm::radians(cfgFile["graphicsSettings"]["FOV"].as_float()),
                                                       float(width) / float(height),
                                                       cfgFile["graphicsSettings"]["nearPlaneDistance"].as_float(),
                                                       cfgFile["graphicsSettings"]["farPlaneDistance"].as_float());
        engine_ptr->GetECSwrapperPtr()->AddComponent(cameraComp_uptr.get());
    }

    {
        modelDrawComp_uptr = std::make_unique<ModelDrawComp>(engine_ptr->GetECSwrapperPtr());
        engine_ptr->GetECSwrapperPtr()->AddComponent(modelDrawComp_uptr.get());
    }

    {
        dynamicMeshComp_uptr = std::make_unique<DynamicMeshComp>(engine_ptr->GetECSwrapperPtr(),
                                                                 dynamicMeshes_uptr.get(),
                                                                 meshesOfNodes_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(dynamicMeshComp_uptr.get());
    }
}

void Graphics::LoadModel(const tinygltf::Model& in_model, std::string model_images_folder)
{
    printf("--Adding model textures and materials\n");
    materialsOfPrimitives_uptr->AddMaterialsOfModel(in_model, model_images_folder);

    printf("--Adding model skins\n");
    skinsOfMeshes_uptr->AddSkinsOfModel(in_model);

    printf("--Adding model meshes\n");
    meshesOfNodes_uptr->AddMeshesOfModel(in_model);
}

void Graphics::EndModelsLoad()
{
    // Flashing device
    materialsOfPrimitives_uptr->FlashDevice(graphicsQueue);
    primitivesOfMeshes_uptr->FlashDevice(graphicsQueue);
    skinsOfMeshes_uptr->FlashDevice(graphicsQueue);
    dynamicMeshes_uptr->FlashDevice();

    // Create primitives sets (shaders-pipelines for each kind of primitive)
    printf("-Initializing \"Texture Pass\" primitives set\n");
    for(size_t i = 0; i != primitivesOfMeshes_uptr->GetPrimitivesCount(); ++i)
    {
        const PrimitiveInfo& this_primitiveInfo = primitivesOfMeshes_uptr->GetPrimitiveInfo(i);
        const MaterialAbout& this_material = materialsOfPrimitives_uptr->GetMaterialAbout(this_primitiveInfo.material);

        bool is_skin = this_primitiveInfo.jointsCount != 0;

        std::vector<std::pair<std::string, std::string>> shadersDefinitionStringPairs = this_material.definitionStringPairs;
        shadersDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(maxInstances));

        // Pipeline layout
        vk::PipelineLayout this_pipeline_layout;
        {
            vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

            std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
            descriptor_sets_layouts.emplace_back(cameraDescriptorSetLayout);
            descriptor_sets_layouts.emplace_back(matricesDescriptorSetLayout);
            descriptor_sets_layouts.emplace_back(materialsOfPrimitives_uptr->GetDescriptorSetLayout());
            pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

            std::vector<vk::PushConstantRange> push_constant_range;
            push_constant_range.emplace_back(vk::ShaderStageFlagBits::eVertex, 0, 4);
            push_constant_range.emplace_back(vk::ShaderStageFlagBits::eFragment, 4, 4);
            pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

            this_pipeline_layout = pipelinesFactory_uptr->GetPipelineLayout(pipeline_layout_create_info).first;
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

            if (this_primitiveInfo.normalOffset != -1) {
                vertex_input_binding_descriptions.emplace_back(binding_index,
                                                               uint32_t(4 * sizeof(float)),
                                                               vk::VertexInputRate::eVertex);
                vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                                 vk::Format::eR32G32B32A32Sfloat,
                                                                 0);

                shadersDefinitionStringPairs.emplace_back("VERT_NORMAL", "");
                shadersDefinitionStringPairs.emplace_back("VERT_NORMAL_LOCATION", std::to_string(location_index));
                ++binding_index; ++location_index;
            }

            if (this_primitiveInfo.tangentOffset != -1) {
                vertex_input_binding_descriptions.emplace_back(binding_index,
                                                               uint32_t(4 * sizeof(float)),
                                                               vk::VertexInputRate::eVertex);
                vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                                 vk::Format::eR32G32B32A32Sfloat,
                                                                 0);

                shadersDefinitionStringPairs.emplace_back("VERT_TANGENT", "");
                shadersDefinitionStringPairs.emplace_back("VERT_TANGENT_LOCATION", std::to_string(location_index));
                ++binding_index; ++location_index;
            }

            if (this_primitiveInfo.texcoordsOffset != -1) {
                vertex_input_binding_descriptions.emplace_back(binding_index,
                                                               uint32_t(this_primitiveInfo.texcoordsCount * 2 * sizeof(float)),
                                                               vk::VertexInputRate::eVertex);

                for (size_t index = 0; index != this_primitiveInfo.texcoordsCount; ++index) {
                    vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                                     vk::Format::eR32G32Sfloat,
                                                                     uint32_t(index * 2 * sizeof(float)));
                    shadersDefinitionStringPairs.emplace_back("VERT_TEXCOORD" + std::to_string(index), "");
                    shadersDefinitionStringPairs.emplace_back("VERT_TEXCOORD" + std::to_string(index) + "_LOCATION", std::to_string(location_index));
                    ++location_index;
                }
                ++binding_index;
            }

            if (this_primitiveInfo.colorOffset != -1) {
                vertex_input_binding_descriptions.emplace_back(binding_index,
                                                               uint32_t(4 * sizeof(float)),
                                                               vk::VertexInputRate::eVertex);
                vertex_input_attribute_descriptions.emplace_back(location_index, binding_index,
                                                                 vk::Format::eR32G32B32A32Sfloat,
                                                                 0);
                shadersDefinitionStringPairs.emplace_back("VERT_COLOR0", "");
                shadersDefinitionStringPairs.emplace_back("VERT_COLOR0_LOCATION", std::to_string(location_index));
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

            uint32_t width = engine_ptr->GetSwapchainCreateInfo().imageExtent.width;
            uint32_t height = engine_ptr->GetSwapchainCreateInfo().imageExtent.height;

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

            ShadersSpecs shaders_specs {"Texture-Pass Shaders", shadersDefinitionStringPairs};
            ShadersSet shader_set = shadersSetsFamiliesCache_uptr->GetShadersSet(shaders_specs);

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

            this_pipeline = pipelinesFactory_uptr->GetPipeline(pipeline_create_info).first;
        }

        primitivesPipelineLayouts.emplace_back(this_pipeline_layout);
        primitivesPipelines.emplace_back(this_pipeline);
    }
}

void Graphics::ToggleCullingDebugging()
{
    cameraComp_uptr->ToggleCullingDebugging();
}



