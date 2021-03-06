#include "Graphics/Graphics.h"
#include "Geometry/FrustumCulling.h"
#include "Engine.h"

#include <utility>
#include <cassert>

#include "misc/swapchain_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/semaphore_create_info.h"
#include "misc/fence_create_info.h"
#include "misc/framebuffer_create_info.h"
#include "misc/render_pass_create_info.h"

Graphics::Graphics(Engine* in_engine_ptr, configuru::Config& in_cfgFile, Anvil::BaseDevice* in_device_ptr, Anvil::Swapchain* in_swapchain_ptr,
                   uint32_t windowWidth, uint32_t windowHeight, uint32_t swapchainImagesCount)
    :
    cfgFile(in_cfgFile),
    swapchainImagesCount(swapchainImagesCount),
    lastSemaphoreUsed(0),
    engine_ptr(in_engine_ptr),
    device_ptr(in_device_ptr),
    swapchain_ptr(in_swapchain_ptr),
    windowWidth(windowWidth),
    windowHeight(windowHeight)
{
    printf("Initializing camera buffers\n");
    InitCameraBuffers();
    printf("Initializing camera descriptor set\n");
    InitCameraDsg();
    printf("Initializing GPU images (z-buffers etc)\n");
    InitImages();
    printf("Initializing framebuffers\n");
    InitFramebuffers();
    printf("Initializing renderpasses\n");
    InitRenderpasses();
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
    printf("Initializing graphics oriented components\n");
    InitGraphicsComponents();
}

Graphics::~Graphics()
{
    Anvil::Vulkan::vkDeviceWaitIdle(device_ptr->get_device_vk());

    cmdBuffers_uptrs.clear();

    frameSignalSemaphores_uptrs.clear();
    frameWaitSemaphores_uptrs.clear();
    framebuffers_uptrs.clear();

    renderpass_uptr.reset();
    depthImage_uptr.reset();
    depthImageView_uptr.reset();

    meshesOfNodes_uptr.reset();
    texturesOfMaterials_uptr.reset();
    materialsOfPrimitives_uptr.reset();
    shadersSetsFamiliesCache_uptr.reset();
    pipelinesFactory_uptr.reset();
    primitivesOfMeshes_uptr.reset();

    cameraDescriptorSetGroup_uptr.reset();

    cameraBuffer_uptrs.clear();
}

void Graphics::DrawFrame()
{
    Anvil::Semaphore*               curr_frame_signal_semaphore_ptr = nullptr;
    Anvil::Semaphore*               curr_frame_wait_semaphore_ptr = nullptr;
    static size_t                   n_frames_rendered = 0;
    uint32_t                        n_swapchain_image;
    Anvil::Queue*                   present_queue_ptr = device_ptr->get_universal_queue(0);
    const Anvil::PipelineStageFlags wait_stage_mask = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    lastSemaphoreUsed = (lastSemaphoreUsed + 1) % swapchainImagesCount;

    curr_frame_signal_semaphore_ptr = frameSignalSemaphores_uptrs[lastSemaphoreUsed].get();
    curr_frame_wait_semaphore_ptr = frameWaitSemaphores_uptrs[lastSemaphoreUsed].get();

    /* Determine the semaphore which the swapchain image */
    {
        const auto acquire_result = swapchain_ptr->acquire_image(curr_frame_wait_semaphore_ptr,
                                                                 &n_swapchain_image,
                                                                 false); /* in_should_block */

        ANVIL_REDUNDANT_VARIABLE_CONST(acquire_result);
        anvil_assert(acquire_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    // TODO: better waiting
    Anvil::Vulkan::vkDeviceWaitIdle(device_ptr->get_device_vk());

    {
        ViewportFrustum camera_viewport_frustum = cameraComp_uptr->GetBindedCameraEntity()->cameraViewportFrustum;

        struct
        {
            glm::mat4 perspective_matrix;
            glm::mat4 view_matrix;
        } data;

        data.perspective_matrix = camera_viewport_frustum.GetPerspectiveMatrix();
        data.view_matrix = camera_viewport_frustum.GetViewMatrix();

        cameraBuffer_uptrs[n_swapchain_image]->write(0,
                                                     2 * sizeof(glm::mat4x4),
                                                     &data,
                                                     present_queue_ptr);
    }

    {
        skinsOfMeshes_uptr->EndRecodingAndFlash(n_swapchain_image, present_queue_ptr);
    }


    RecordCommandBuffer(n_swapchain_image);

    present_queue_ptr->submit(
        Anvil::SubmitInfo::create(cmdBuffers_uptrs[n_swapchain_image].get(),
                                  1,
                                  &curr_frame_signal_semaphore_ptr,
                                  1,
                                  &curr_frame_wait_semaphore_ptr,
                                  &wait_stage_mask,
                                  false /* should_block */));

    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;
        present_queue_ptr->present(swapchain_ptr,
                                   n_swapchain_image,
                                   1, /* n_wait_semaphores */
                                   &curr_frame_signal_semaphore_ptr,
                                   &present_result);

        anvil_assert(present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    n_frames_rendered++;
}

void Graphics::RecordCommandBuffer(uint32_t swapchainImageIndex)
{
    const uint32_t                 universal_queue_family_index = device_ptr->get_universal_queue(0)->get_queue_family_index();

    Anvil::PrimaryCommandBuffer* cmd_buffer_ptr = cmdBuffers_uptrs[swapchainImageIndex].get();

    /* Reset command buffer*/
    cmd_buffer_ptr->reset(false);

    /* Start recording commands */
    cmd_buffer_ptr->start_recording(true,  /* one_time_submit          */
                                    false); /* simultaneous_use_allowed */

    {
        /* Switch the swap-chain image to the color_attachment_optimal image layout */
        Anvil::ImageSubresourceRange  image_subresource_range;
        image_subresource_range.aspect_mask = Anvil::ImageAspectFlagBits::COLOR_BIT;
        image_subresource_range.base_array_layer = 0;
        image_subresource_range.base_mip_level = 0;
        image_subresource_range.layer_count = 1;
        image_subresource_range.level_count = 1;

        Anvil::ImageBarrier image_barrier(Anvil::AccessFlagBits::NONE,                              /* source_access_mask       */
                                          Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,        /* destination_access_mask  */
                                          Anvil::ImageLayout::UNDEFINED,                            /* old_image_layout */
                                          Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,             /* new_image_layout */
                                          universal_queue_family_index,
                                          universal_queue_family_index,
                                          swapchain_ptr->get_image(swapchainImageIndex),
                                          image_subresource_range);

        cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TOP_OF_PIPE_BIT,              /* src_stage_mask                 */
                                                Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,  /* dst_stage_mask                 */
                                                Anvil::DependencyFlagBits::NONE,
                                                0,                                                          /* in_memory_barrier_count        */
                                                nullptr,                                                    /* in_memory_barrier_ptrs         */
                                                0,                                                          /* in_buffer_memory_barrier_count */
                                                nullptr,                                                    /* in_buffer_memory_barrier_ptrs  */
                                                1,                                                          /* in_image_memory_barrier_count  */
                                                &image_barrier);
    }

    {
        VkClearValue  clear_values[2];
        clear_values[0].color.float32[0] = 0.0f;
        clear_values[0].color.float32[1] = 0.0f;
        clear_values[0].color.float32[2] = 0.0f;
        clear_values[0].color.float32[3] = 1.0f;
        clear_values[1].depthStencil.depth = 1.0f;

        VkRect2D  render_area;
        render_area.extent.width = windowWidth;
        render_area.extent.height = windowHeight;
        render_area.offset.x = 0;
        render_area.offset.y = 0;

        cmd_buffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                 clear_values,
                                                 framebuffers_uptrs[swapchainImageIndex].get(),
                                                 render_area,
                                                 renderpass_uptr.get(),
                                                 Anvil::SubpassContents::INLINE);

        Drawer opaque_drawer("Texture-Pass",
                             primitivesOfMeshes_uptr.get(),
                             device_ptr);

        Drawer transparent_drawer("Texture-Pass",
                                  primitivesOfMeshes_uptr.get(),
                                  device_ptr);

        {
            ViewportFrustum camera_viewport_frustum = cameraComp_uptr->GetBindedCameraEntity()->cullingViewportFrustum;

            FrustumCulling frustum_culling;
            frustum_culling.SetFrustumPlanes(camera_viewport_frustum.GetWorldSpacePlanesOfFrustum());

            DrawRequestsBatch draw_requests = modelDrawComp_uptr->DrawUsingFrustumCull(meshesOfNodes_uptr.get(),
                                                                                       primitivesOfMeshes_uptr.get(),
                                                                                       &frustum_culling);

            opaque_drawer.AddDrawRequests(draw_requests.opaqueDrawRequests);
            transparent_drawer.AddDrawRequests(draw_requests.transparentDrawRequests);
        }

        {
            DescriptorSetsPtrsCollection this_description_set_collection;
            this_description_set_collection.camera_description_set_ptr = cameraDescriptorSetGroup_uptr->get_descriptor_set(swapchainImageIndex);
            this_description_set_collection.skin_description_set_ptr = skinsOfMeshes_uptr->GetDescriptorSetPtr(swapchainImageIndex);
            this_description_set_collection.materials_description_set_ptr = materialsOfPrimitives_uptr->GetDescriptorSetPtr();

            opaque_drawer.DrawCallRequests(cmd_buffer_ptr, "Texture-Pass", this_description_set_collection);
            transparent_drawer.DrawCallRequests(cmd_buffer_ptr, "Texture-Pass", this_description_set_collection);
        }

       

        cmd_buffer_ptr->record_end_render_pass();
    }

    cmd_buffer_ptr->stop_recording();
}

void Graphics::InitCameraBuffers()
{
    Anvil::MemoryAllocatorUniquePtr   allocator_ptr;

    const Anvil::MemoryFeatureFlags   required_feature_flags = Anvil::MemoryFeatureFlagBits::NONE;

    allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    cameraBuffer_uptrs.resize(swapchainImagesCount);

    for (uint32_t i = 0; i < swapchainImagesCount; i++)
    {
        {
            auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                2 * sizeof(glm::mat4),
                Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                Anvil::SharingMode::EXCLUSIVE,
                Anvil::BufferCreateFlagBits::NONE,
                Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

            cameraBuffer_uptrs[i] = Anvil::Buffer::create(std::move(create_info_ptr));

            cameraBuffer_uptrs[i]->set_name("Camera matrix buffer " + std::to_string(i));
        }

        allocator_ptr->add_buffer(cameraBuffer_uptrs[i].get(),
                                  required_feature_flags);
    }
}

void Graphics::InitCameraDsg()
{
    Anvil::DescriptorSetGroupUniquePtr new_dsg_ptr;

    std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> new_dsg_create_info_ptr;
    new_dsg_create_info_ptr.resize(swapchainImagesCount);

    for(uint32_t i = 0; i < swapchainImagesCount; i++)
    {
        new_dsg_create_info_ptr[i] = Anvil::DescriptorSetCreateInfo::create();

        new_dsg_create_info_ptr[i]->add_binding(0, /* in_binding */
                                                Anvil::DescriptorType::UNIFORM_BUFFER,
                                                1, /* in_n_elements */
                                                Anvil::ShaderStageFlagBits::VERTEX_BIT);

        new_dsg_create_info_ptr[i]->add_binding(1, /* in_binding */
                                                Anvil::DescriptorType::UNIFORM_BUFFER,
                                                1, /* in_n_elements */
                                                Anvil::ShaderStageFlagBits::VERTEX_BIT);
    }


    new_dsg_ptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                    { new_dsg_create_info_ptr },
                                                    false); /* in_releaseable_sets */

    for (uint32_t i = 0; i < swapchainImagesCount; i++)
    {
        new_dsg_ptr->set_binding_item(i, /* n_set         */
                                      0, /* binding_index */
                                      Anvil::DescriptorSet::UniformBufferBindingElement(cameraBuffer_uptrs[i].get(),
                                                                                        0,
                                                                                        sizeof(glm::mat4)));

        new_dsg_ptr->set_binding_item(i, /* n_set         */
                                      1, /* binding_index */
                                      Anvil::DescriptorSet::UniformBufferBindingElement(cameraBuffer_uptrs[i].get(),
                                                                                        sizeof(glm::mat4),
                                                                                        sizeof(glm::mat4)));
    }

    cameraDescriptorSetGroup_uptr = std::move(new_dsg_ptr);
}

void Graphics::InitImages()
{
    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(device_ptr,
                                                                    Anvil::ImageType::_2D,
                                                                    Anvil::Format::D32_SFLOAT,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                    windowWidth,
                                                                    windowHeight,
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false, /* in_use_full_mipmap_chain */
                                                                    Anvil::MemoryFeatureFlagBits::NONE,
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /* in_final_image_layout */
                                                                    nullptr); /* in_mipmaps_ptr */

        depthImage_uptr = Anvil::Image::create(std::move(create_info_ptr));
    }
    {
        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(device_ptr,
                                                                     depthImage_uptr.get(),
                                                                     0, /* n_base_layer */
                                                                     0, /* n_base_mipmap_level */
                                                                     1, /* n_mipmaps */
                                                                     Anvil::ImageAspectFlagBits::DEPTH_BIT,
                                                                     depthImage_uptr->get_create_info_ptr()->get_format(),
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY);

        depthImageView_uptr = Anvil::ImageView::create(std::move(create_info_ptr));
    }
}

void Graphics::InitFramebuffers()
{

    for (uint32_t n_swapchain_image = 0;
         n_swapchain_image < swapchainImagesCount;
         ++n_swapchain_image)
    {
        Anvil::FramebufferUniquePtr framebuffer_ptr;

        {
            auto create_info_ptr = Anvil::FramebufferCreateInfo::create(device_ptr,
                                                                        windowWidth,
                                                                        windowHeight,
                                                                        1); /* n_layers */

            create_info_ptr->add_attachment(swapchain_ptr->get_image_view(n_swapchain_image),
                                            nullptr);

            create_info_ptr->add_attachment(depthImageView_uptr.get(),
                                            nullptr);

            framebuffer_ptr = Anvil::Framebuffer::create(std::move(create_info_ptr));
        }

        framebuffer_ptr->set_name("Main framebuffer");


        framebuffers_uptrs.push_back(
            std::move(framebuffer_ptr)
        );

    }
}

void Graphics::InitRenderpasses()
{
        Anvil::RenderPassAttachmentID color_attachment_id;
        Anvil::RenderPassAttachmentID depth_attachment_id;

        Anvil::RenderPassCreateInfoUniquePtr renderpass_create_info_ptr(new Anvil::RenderPassCreateInfo(device_ptr));

        renderpass_create_info_ptr->add_color_attachment(swapchain_ptr->get_create_info_ptr()->get_format(),
                                                         Anvil::SampleCountFlagBits::_1_BIT,
                                                         Anvil::AttachmentLoadOp::CLEAR,
                                                         Anvil::AttachmentStoreOp::STORE,
                                                         Anvil::ImageLayout::UNDEFINED,
                                                         Anvil::ImageLayout::PRESENT_SRC_KHR,
                                                         false, /* may_alias */
                                                         &color_attachment_id);

        renderpass_create_info_ptr->add_depth_stencil_attachment(depthImage_uptr->get_create_info_ptr()->get_format(),
                                                                 Anvil::SampleCountFlagBits::_1_BIT,
                                                                 Anvil::AttachmentLoadOp::CLEAR,
                                                                 Anvil::AttachmentStoreOp::DONT_CARE,
                                                                 Anvil::AttachmentLoadOp::DONT_CARE,
                                                                 Anvil::AttachmentStoreOp::DONT_CARE,
                                                                 Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                 Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                 false, /* may_alias */
                                                                 &depth_attachment_id);

        // texture pass
        renderpass_create_info_ptr->add_subpass(&textureSubpassID);
        renderpass_create_info_ptr->add_subpass_color_attachment(textureSubpassID,
                                                                 Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                 color_attachment_id,
                                                                 0,        /* in_location                      */
                                                                 nullptr); /* in_opt_attachment_resolve_id_ptr */
        renderpass_create_info_ptr->add_subpass_depth_stencil_attachment(textureSubpassID,
                                                                         Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                         depth_attachment_id);


        renderpass_uptr = Anvil::RenderPass::create(std::move(renderpass_create_info_ptr),
                                                    swapchain_ptr);

        renderpass_uptr->set_name("Renderpass for swapchain images");
}

void Graphics::InitSemaphoresAndFences()
{
    for (uint32_t n_semaphore = 0;
         n_semaphore < swapchainImagesCount;
         ++n_semaphore)
    {
        Anvil::SemaphoreUniquePtr new_signal_semaphore_ptr;
        Anvil::SemaphoreUniquePtr new_wait_semaphore_ptr;

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(device_ptr);

            new_signal_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr));

            new_signal_semaphore_ptr->set_name_formatted("Signal semaphore [%d]",
                                                         n_semaphore);
        }

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(device_ptr);

            new_wait_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr));

            new_wait_semaphore_ptr->set_name_formatted("Wait semaphore [%d]",
                                                       n_semaphore);
        }

        frameSignalSemaphores_uptrs.push_back(std::move(new_signal_semaphore_ptr));
        frameWaitSemaphores_uptrs.push_back(std::move(new_wait_semaphore_ptr));
    }
}

void Graphics::InitCommandBuffers()
{
    const Anvil::DeviceType        device_type = device_ptr->get_type();

    const uint32_t                 universal_queue_family_index = device_ptr->get_universal_queue(0)->get_queue_family_index();

    for (size_t i = 0; i < swapchainImagesCount; i++)
    {
        Anvil::PrimaryCommandBufferUniquePtr cmd_buffer_uptr;
        cmd_buffer_uptr = device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_index)->alloc_primary_level_command_buffer();

        cmdBuffers_uptrs.emplace_back(std::move(cmd_buffer_uptr));
    }
}

void Graphics::InitPipelinesFactory()
{
    pipelinesFactory_uptr = std::make_unique<PipelinesFactory>(device_ptr);
}

void Graphics::InitShadersSetsFamiliesCache()
{
    shadersSetsFamiliesCache_uptr = std::make_unique<ShadersSetsFamiliesCache>(device_ptr);

    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Texture-Pass Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "generalShader_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "generalShader_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "General Mipmap Compute Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "generalMipmap_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Image 16bit To 8bit Pass";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "16bitTo8bit_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "16bitTo8bit_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
}

void Graphics::InitMeshesTree()
{
    // Find out how each texture is being used (color, normal, etc..)
    imagesAboutOfTextures_uptr = std::make_unique<ImagesAboutOfTextures>();                                                                                                         //useless afterwards

    mipmapsGenerator_uptr = std::make_unique<MipmapsGenerator>(pipelinesFactory_uptr.get(),                                                                                         //useless afterwards
                                                               shadersSetsFamiliesCache_uptr.get(),
                                                               imagesAboutOfTextures_uptr.get(),
                                                               "Image 16bit To 8bit Pass",
                                                               "General Mipmap Compute Shader", /* baseColor_shadername */
                                                               "General Mipmap Compute Shader", /* occlusionMetallicRoughness_shadername */
                                                               "General Mipmap Compute Shader", /* normal_shadername */
                                                               "General Mipmap Compute Shader", /* emissive_shadername */
                                                               cmdBuffers_uptrs[0].get(),
                                                               device_ptr);

    animationsDataOfNodes_uptr = std::make_unique<AnimationsDataOfNodes>();

    texturesOfMaterials_uptr = std::make_unique<TexturesOfMaterials>(cfgFile["graphicsSettings"]["useMipmaps"].as_bool(), imagesAboutOfTextures_uptr.get(), mipmapsGenerator_uptr.get(), device_ptr);

    materialsOfPrimitives_uptr = std::make_unique<MaterialsOfPrimitives>(texturesOfMaterials_uptr.get(), device_ptr);                                                               //needs flash

    primitivesOfMeshes_uptr = std::make_unique<PrimitivesOfMeshes>(pipelinesFactory_uptr.get(), shadersSetsFamiliesCache_uptr.get(), materialsOfPrimitives_uptr.get(), device_ptr); //needs flash

    skinsOfMeshes_uptr = std::make_unique<SkinsOfMeshes>(device_ptr, swapchainImagesCount, 64 * 1024);

    meshesOfNodes_uptr = std::make_unique<MeshesOfNodes>(primitivesOfMeshes_uptr.get(), device_ptr);
}

void Graphics::InitGraphicsComponents()
{
    {
        animationActorComp_uptr = std::make_unique<AnimationActorComp>(engine_ptr->GetECSwrapperPtr(),
                                                                       animationsDataOfNodes_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(animationActorComp_uptr.get());
    }
    {
        cameraComp_uptr = std::make_unique<CameraComp>(engine_ptr->GetECSwrapperPtr(),
                                                       glm::radians(cfgFile["graphicsSettings"]["FOV"].as_float()),
                                                       static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
                                                       cfgFile["graphicsSettings"]["nearPlaneDistance"].as_float(),
                                                       cfgFile["graphicsSettings"]["farPlaneDistance"].as_float());
        engine_ptr->GetECSwrapperPtr()->AddComponent(cameraComp_uptr.get());
    }

    {
        modelDrawComp_uptr = std::make_unique<ModelDrawComp>(engine_ptr->GetECSwrapperPtr());
        engine_ptr->GetECSwrapperPtr()->AddComponent(modelDrawComp_uptr.get());
    }

    {
        skinComp_uptr = std::make_unique<SkinComp>(engine_ptr->GetECSwrapperPtr(),
                                                   skinsOfMeshes_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(skinComp_uptr.get());
    }
}

void Graphics::LoadModel(const tinygltf::Model& in_model, std::string in_model_images_folder)
{
    // Find out how each texture is being used (color, normal, etc..)
    printf("--Adding model to ImagesAboutOfTextures\n");
    imagesAboutOfTextures_uptr->AddImagesUsageOfModel(in_model);

    // Edit textures and copy to GPU
    printf("--Adding model to TexturesOfMaterials\n");
    texturesOfMaterials_uptr->AddTexturesOfModel(in_model, in_model_images_folder);

    // Create materials description sets
    printf("--Adding model to MaterialsOfPrimitives\n");
    materialsOfPrimitives_uptr->AddMaterialsOfModel(in_model);

    // Initialize models-primtives handler to GPU
    printf("--Adding model to PrimitivesOfMeshes\n");
  //primitivesOfMeshes_uptr-> collects primitives from meshesOfNodes_uptr

    printf("--Adding model to SkinsOfMeshes\n");
    skinsOfMeshes_uptr->AddSkinsOfModel(in_model);

    // For every mesh copy primitives of it to GPU
    printf("--Adding model to MeshesOfNodes\n");
    meshesOfNodes_uptr->AddMeshesOfModel(in_model);
}

void Graphics::EndModelsLoad()
{
    imagesAboutOfTextures_uptr.reset();
    mipmapsGenerator_uptr.reset();

    // Flashing device
    primitivesOfMeshes_uptr->FlashDevice();
    materialsOfPrimitives_uptr->FlashDevice();
    skinsOfMeshes_uptr->FlashDevice();

    DescriptorSetsCreateInfosPtrsCollection this_descriptor_sets_create_infos_ptrs_collection;
    this_descriptor_sets_create_infos_ptrs_collection.camera_description_set_create_info_ptr = cameraDescriptorSetGroup_uptr->get_descriptor_set_create_info(0);    // All camera sets are the same from create info standpoint
    this_descriptor_sets_create_infos_ptrs_collection.skins_description_set_create_info_ptr = skinsOfMeshes_uptr->GetDescriptorSetCreateInfoPtr();
    this_descriptor_sets_create_infos_ptrs_collection.materials_description_set_create_info_ptr = materialsOfPrimitives_uptr->GetDescriptorSetCreateInfoPtr();

    // Create primitives sets (shaders-pipelines for each kind of primitive)
    {
        printf("-Initializing \"Texture Pass\" primitives set\n");

        PrimitivesSetSpecs this_primitives_set_specs;
        this_primitives_set_specs.primitivesSetName = "Texture-Pass";
        this_primitives_set_specs.useDepthWrite = true;
        this_primitives_set_specs.depthCompare = Anvil::CompareOp::LESS;
        this_primitives_set_specs.useMaterial = true;
        this_primitives_set_specs.shaderSpecs.shadersSetFamilyName = "Texture-Pass Shaders";

        this_primitives_set_specs.shaderSpecs.definitionValuePairs.emplace_back(std::pair("INVERSE_BIND_COUNT", static_cast<int>(skinsOfMeshes_uptr->GetMaxCountOfInverseBindMatrices())));
        this_primitives_set_specs.shaderSpecs.definitionValuePairs.emplace_back(std::pair("NODES_MATRICES_COUNT", static_cast<int>(skinsOfMeshes_uptr->GetMaxCountOfNodesMatrices())));

        primitivesOfMeshes_uptr->InitPrimitivesSet(this_primitives_set_specs, this_descriptor_sets_create_infos_ptrs_collection, renderpass_uptr.get(), textureSubpassID);

    }
}

MeshesOfNodes* Graphics::GetMeshesOfNodesPtr()
{
    assert(meshesOfNodes_uptr.get());
    return meshesOfNodes_uptr.get();
}

SkinsOfMeshes* Graphics::GetSkinsOfMeshesPtr()
{
    assert(skinsOfMeshes_uptr.get());
    return skinsOfMeshes_uptr.get();
}

AnimationsDataOfNodes* Graphics::GetAnimationsDataOfNodesPtr()
{
    assert(animationsDataOfNodes_uptr.get());
    return animationsDataOfNodes_uptr.get();
}

void Graphics::ToggleCullingDebugging()
{
    cameraComp_uptr->ToggleCullingDebugging();
}