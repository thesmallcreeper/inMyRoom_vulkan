#define GLM_FORECE_DEPTH_ZERO_TO_ONE

#include "Graphics.h"

#include "config.h"

#include "misc/window_factory.h"
#include "misc/swapchain_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/semaphore_create_info.h"
#include "misc/glsl_to_spirv.h"
#include "misc/framebuffer_create_info.h"
#include "misc/render_pass_create_info.h"
#include "misc/instance_create_info.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/device.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/descriptor_set_layout.h"
#include "wrappers/framebuffer.h"
#include "wrappers/event.h"
#include "wrappers/instance.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/render_pass.h"
#include "wrappers/semaphore.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/shader_module.h"
#include "wrappers/swapchain.h"

#ifdef _DEBUG
    #define ENABLE_VALIDATION
#endif

Graphics::Graphics(configuru::Config& in_cfgFile)
    :cfgFile(in_cfgFile),
     m_n_swapchain_images(cfgFile["graphicsSettings"]["swapchain_images"].as_integer<unsigned int>()),
     m_fov_deg(cfgFile["graphicsSettings"]["FOV"].as_float()),
     m_n_last_semaphore_used(0)
{

}

Graphics::~Graphics()
{
    deinit();
}

void Graphics::init()
{
    load_scene();

    init_vulkan();
    init_window_with_async_input_ptr();
    init_swapchain();

    init_camera_buffers();
    init_images();
    init_framebuffers();
    init_renderpasses();
    init_scene();
    init_semaphores();

    init_command_buffers();
}

void Graphics::deinit()
{
    Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk());

    m_cmd_buffers.clear();

    m_frame_signal_semaphores.clear();
    m_frame_wait_semaphores.clear();
    m_framebuffers.clear();

    m_renderpass_ptr.reset();
    m_depth_image_ptr.reset();
    m_depth_image_view_ptr.reset();

    nodesMeshes_ptr.reset();
    materialsTextures_ptr.reset();
    primitivesMaterials_ptr.reset();
    primitivesShaders_ptr.reset();
    primitivesPipelines_ptr.reset();
    meshesPrimitives_ptr.reset();

    m_dsg_ptr.reset();

    sceneNodes_ptr.reset();

    m_camera_buffer_ptr.reset();
    m_perspective_buffer_ptr.reset();

    m_swapchain_ptr.reset();
    m_rendering_surface_ptr.reset();

    m_device_ptr.reset();
    m_instance_ptr.reset();

    window_with_async_input_ptr.reset();
}

void Graphics::draw_frame()
{
    Anvil::Semaphore*               curr_frame_signal_semaphore_ptr = nullptr;
    Anvil::Semaphore*               curr_frame_wait_semaphore_ptr = nullptr;
    static uint32_t                 n_frames_rendered = 0;
    uint32_t                        n_swapchain_image;
    Anvil::Queue*                   present_queue_ptr = m_device_ptr->get_universal_queue(0);
    Anvil::Semaphore*               present_wait_semaphore_ptr = nullptr;
    const Anvil::PipelineStageFlags wait_stage_mask = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

    /* Determine the signal + wait semaphores to use for drawing this frame */
    m_n_last_semaphore_used = (m_n_last_semaphore_used + 1) % m_n_swapchain_images;

    curr_frame_signal_semaphore_ptr = m_frame_signal_semaphores[m_n_last_semaphore_used].get();
    curr_frame_wait_semaphore_ptr = m_frame_wait_semaphores[m_n_last_semaphore_used].get();

    present_wait_semaphore_ptr = curr_frame_signal_semaphore_ptr;

    /* Determine the semaphore which the swapchain image */
    {
        const auto acquire_result = m_swapchain_ptr->acquire_image(curr_frame_wait_semaphore_ptr,
                                                                   &n_swapchain_image,
                                                                   true); /* in_should_block */

        ANVIL_REDUNDANT_VARIABLE_CONST(acquire_result);
        anvil_assert(acquire_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }

    Anvil::Vulkan::vkDeviceWaitIdle(m_device_ptr->get_device_vk());

    glm::mat4x4 camera_matrix = camera->getLookAtMatrix();

    m_camera_buffer_ptr->write(0,
                               sizeof(glm::mat4x4),
                               &camera_matrix);

    present_queue_ptr->submit(
        Anvil::SubmitInfo::create(m_cmd_buffers[n_swapchain_image].get(),
                                  1,
                                  &curr_frame_signal_semaphore_ptr,
                                  1,
                                  &curr_frame_wait_semaphore_ptr,
                                  &wait_stage_mask,
                                  false /* should_block */)
    );

    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;
        present_queue_ptr->present(m_swapchain_ptr.get(),
                                   n_swapchain_image,
                                   1, /* n_wait_semaphores */
                                   &present_wait_semaphore_ptr,
                                   &present_result);

        anvil_assert(present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }


}

void Graphics::load_scene()
{
    std::string path = cfgFile["sceneInput"]["path"].as_string();

    tinygltf::TinyGLTF loader;
    std::string err;
    std::string warn;

    std::string ext = GetFilePathExtension(path);

    bool ret = false;
    if (ext.compare("glb") == 0) {
        // assume binary glTF.
        ret = loader.LoadBinaryFromFile(&model, &err, &warn, path.c_str());
    }
    else {
        // assume ascii glTF.
        ret = loader.LoadASCIIFromFile(&model, &err, &warn, path.c_str());
    }

    if (!warn.empty()) {
        printf("Warn: %s\n", warn.c_str());
    }

    if (!err.empty()) {
        printf("ERR: %s\n", err.c_str());
    }
    if (!ret) {
        printf("Failed to load .glTF : %s\n", path.c_str());
        exit(-1);
    }
}
void Graphics::init_vulkan()
{
    /* Create a Vulkan instance */
    auto create_info_ptr = Anvil::InstanceCreateInfo::create("inMyRoom_vulkan",  /* app_name */
                                                             "inMyRoom_vulkan",  /* engine_name */
#ifdef ENABLE_VALIDATION
                                                             std::bind(&Graphics::on_validation_callback,
                                                             this,
                                                             std::placeholders::_1,
                                                             std::placeholders::_2
                                                             ),
#else
                                                             Anvil::DebugCallbackFunction(),
#endif
                                                             false);             /* in_mt_safe */

    m_instance_ptr = Anvil::Instance::create(std::move(create_info_ptr));

    /* Determine which extensions we need to request for */
    {
        std::vector<std::string> vulkan_layers;
        vulkan_layers.emplace_back("VK_LAYER_LUNARG_api_dump");
        /* Create a Vulkan device */
        m_device_ptr = Anvil::SGPUDevice::create(m_instance_ptr->get_physical_device(0),
                                                 true, /* in_enable_shader_module_cache */
                                                 Anvil::DeviceExtensionConfiguration(),
                                                 vulkan_layers,                                                                      /* in_layers                               */
                                                 false,                                             /* in_transient_command_buffer_allocs_only */
                                                 false);                                            /* in_support_resettable_command_buffers   */
    }
}

void Graphics::init_window_with_async_input_ptr()
{
    #ifdef _WIN32
        const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_SYSTEM;
    #else
        const Anvil::WindowPlatform platform = Anvil::WINDOW_PLATFORM_XCB;
    #endif

    window_with_async_input_ptr = std::make_unique<WindowWithAsyncInput>(platform,
                                                                         "inMyRoom_vulkan",
                                                                         cfgFile["graphicsSettings"]["xRes"].as_integer<unsigned int>(),
                                                                         cfgFile["graphicsSettings"]["yRes"].as_integer<unsigned int>(),
                                                                         true);
}

void Graphics::init_swapchain()
{
    static const Anvil::Format          swapchain_format(Anvil::Format::B8G8R8A8_UNORM);
    static const Anvil::PresentModeKHR  swapchain_present_mode(Anvil::PresentModeKHR::FIFO_KHR);
    static const Anvil::ImageUsageFlags swapchain_usage(Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT | Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT);

    m_rendering_surface_ptr = Anvil::RenderingSurface::create(m_instance_ptr.get(),
                                                              m_device_ptr.get(),
                                                              window_with_async_input_ptr -> m_window_ptr.get());

    m_rendering_surface_ptr->set_name("Main rendering surface");


    switch (m_device_ptr->get_type())
    {
    case Anvil::DeviceType::SINGLE_GPU:
    {
        Anvil::SGPUDevice* sgpu_device_ptr(dynamic_cast<Anvil::SGPUDevice*>(m_device_ptr.get()));

        m_swapchain_ptr = sgpu_device_ptr->create_swapchain(m_rendering_surface_ptr.get(),
            window_with_async_input_ptr -> m_window_ptr.get(),
            swapchain_format,
            Anvil::ColorSpaceKHR::SRGB_NONLINEAR_KHR,
            swapchain_present_mode,
            swapchain_usage,
            m_n_swapchain_images);

        /* Cache the queue we are going to use for presentation */
        const std::vector<uint32_t>* present_queue_fams_ptr = nullptr;

        if (!m_rendering_surface_ptr->get_queue_families_with_present_support(sgpu_device_ptr->get_physical_device(),
            &present_queue_fams_ptr))
        {
            anvil_assert_fail();
        }

        m_present_queue_ptr = sgpu_device_ptr->get_queue_for_queue_family_index(present_queue_fams_ptr->at(0),
            0); /* in_n_queue */

        break;
    }

    default:
    {
        anvil_assert(false);
    }
    }
}


void Graphics::init_camera_buffers()
{
    Anvil::MemoryAllocatorUniquePtr   allocator_ptr;

    const Anvil::DeviceType           device_type(m_device_ptr->get_type());

    const Anvil::MemoryFeatureFlags   required_feature_flags = Anvil::MemoryFeatureFlagBits::NONE;

    allocator_ptr = Anvil::MemoryAllocator::create_oneshot(m_device_ptr.get());

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        sizeof(glm::mat4),
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

        m_camera_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

        m_camera_buffer_ptr->set_name("Camera matrix buffer");
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(m_device_ptr.get(),
                                                                        sizeof(glm::mat4),
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

        m_perspective_buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

        m_perspective_buffer_ptr->set_name("Camera matrix buffer");
    }

    allocator_ptr->add_buffer(m_camera_buffer_ptr.get(),
                              required_feature_flags);

    allocator_ptr->add_buffer(m_perspective_buffer_ptr.get(),
                              required_feature_flags);

    // Camera buffer is being updated every frame

    glm::mat4x4 perspective_matrix = glm::perspective(glm::radians(m_fov_deg), (float)window_with_async_input_ptr->m_window_ptr->get_width_at_creation_time() / (float)window_with_async_input_ptr->m_window_ptr->get_height_at_creation_time(), 1.0f, 100.0f) * glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,
                                                                                                                                                                                                                                                                          0.0f, -1.0f, 0.0f, 0.0f,
                                                                                                                                                                                                                                                                          0.0f, 0.0f, 1.0f, 0.0f,
                                                                                                                                                                                                                                                                          0.0f, 0.0f, 0.0f, 1.0f);

    m_perspective_buffer_ptr->write(0,
                                    sizeof(glm::mat4x4),
                                    &perspective_matrix);

}

void Graphics::init_scene()
{
    const tinygltf::Scene &scene = model.scenes[0];

    materialsTextures_ptr = std::make_unique <MaterialsTextures>(model, cfgFile["sceneInput"]["imagesFolder"].as_string(), m_device_ptr.get());
    primitivesMaterials_ptr = std::make_unique<PrimitivesMaterials>(model, materialsTextures_ptr.get(), m_device_ptr.get());
    {
        std::vector<ShaderSetFamilyInitInfo> shaderSetsInitInfos;
        ShaderSetFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "General Mesh";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "generalMesh_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "generalMesh_glsl.vert";

        shaderSetsInitInfos.emplace_back(this_shaderSetInitInfo);

        primitivesShaders_ptr = std::make_unique<PrimitivesShaders>(shaderSetsInitInfos, m_device_ptr.get());
    }
    primitivesPipelines_ptr = std::make_unique<PrimitivesPipelines>(m_device_ptr.get());    
    sceneNodes_ptr = std::make_unique<SceneNodes>(model, scene, m_device_ptr.get());

    init_spacial_dsg();

    meshesPrimitives_ptr = std::make_unique<MeshesPrimitives>(primitivesPipelines_ptr.get(), primitivesShaders_ptr.get(), primitivesMaterials_ptr.get(), m_device_ptr.get());
    nodesMeshes_ptr = std::make_unique<NodesMeshes>(model, meshesPrimitives_ptr.get(), m_device_ptr.get());

    meshesPrimitives_ptr->FlashBuffersToDevice();
    {
        ShadersSpecs this_shaders_specs;
        this_shaders_specs.shadersSetFamilyName = "General Mesh";
        this_shaders_specs.definitionValuePairs.emplace_back(std::make_pair("N_MESHIDS", sceneNodes_ptr->globalTRSmatrixesCount));
        PrimitivesSetIndex = meshesPrimitives_ptr->InitPrimitivesSet(this_shaders_specs, true, m_dsg_ptr->get_descriptor_set_create_info(), m_renderpass_ptr.get(), m_subpass_id);
    }

    sceneNodes_ptr->BindSceneMeshes(nodesMeshes_ptr.get());
}


void Graphics::init_spacial_dsg()
{
    std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> new_dsg_create_info_ptr;
    new_dsg_create_info_ptr.resize(2);

    Anvil::DescriptorSetGroupUniquePtr new_dsg_ptr;

    new_dsg_create_info_ptr[0] = Anvil::DescriptorSetCreateInfo::create();

    new_dsg_create_info_ptr[0]->add_binding(0, /* in_binding */
                                            Anvil::DescriptorType::STORAGE_BUFFER,
                                            1, /* in_n_elements */
                                            Anvil::ShaderStageFlagBits::VERTEX_BIT);

    new_dsg_create_info_ptr[1] = Anvil::DescriptorSetCreateInfo::create();

    new_dsg_create_info_ptr[1]->add_binding(0, /* in_binding */
                                            Anvil::DescriptorType::UNIFORM_BUFFER,
                                            1, /* in_n_elements */
                                            Anvil::ShaderStageFlagBits::VERTEX_BIT);

    new_dsg_create_info_ptr[1]->add_binding(1, /* in_binding */
                                            Anvil::DescriptorType::UNIFORM_BUFFER,
                                            1, /* in_n_elements */
                                            Anvil::ShaderStageFlagBits::VERTEX_BIT);


    new_dsg_ptr = Anvil::DescriptorSetGroup::create(m_device_ptr.get(),
                                                    { new_dsg_create_info_ptr },
                                                    false); /* in_releaseable_sets */

    new_dsg_ptr->set_binding_item(0, /* n_set         */
                                  0, /* binding_index */
                                  Anvil::DescriptorSet::StorageBufferBindingElement(sceneNodes_ptr->globalTRSmatrixesBuffer.get()));

    new_dsg_ptr->set_binding_item(1, /* n_set         */
                                  0, /* binding_index */
                                  Anvil::DescriptorSet::UniformBufferBindingElement(m_perspective_buffer_ptr.get()));

    new_dsg_ptr->set_binding_item(1, /* n_set         */
                                  1, /* binding_index */
                                  Anvil::DescriptorSet::UniformBufferBindingElement(m_camera_buffer_ptr.get()));

    m_dsg_ptr = std::move(new_dsg_ptr);
}

void Graphics::init_images()
{
    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(m_device_ptr.get(),
                                                                    Anvil::ImageType::_2D,
                                                                    Anvil::Format::D32_SFLOAT,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::DEPTH_STENCIL_ATTACHMENT_BIT,
                                                                    window_with_async_input_ptr->m_window_ptr->get_width_at_creation_time(),
                                                                    window_with_async_input_ptr->m_window_ptr->get_height_at_creation_time(),
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false, /* in_use_full_mipmap_chain */
                                                                    Anvil::MemoryFeatureFlagBits::NONE,
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL, /* in_final_image_layout    */
                                                                    nullptr);                                             /* in_mipmaps_ptr           */

        m_depth_image_ptr = Anvil::Image::create(std::move(create_info_ptr));
    }
    {
        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(m_device_ptr.get(),
                                                                     m_depth_image_ptr.get(),
                                                                     0, /* n_base_layer        */
                                                                     0, /* n_base_mipmap_level */
                                                                     1, /* n_mipmaps           */
                                                                     Anvil::ImageAspectFlagBits::DEPTH_BIT,
                                                                     m_depth_image_ptr->get_create_info_ptr()->get_format(),
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY,
                                                                     Anvil::ComponentSwizzle::IDENTITY);

        m_depth_image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr));
    }
}

void Graphics::init_semaphores()
{
    for (uint32_t n_semaphore = 0;
         n_semaphore < m_n_swapchain_images;
         ++n_semaphore)
    {
        Anvil::SemaphoreUniquePtr new_signal_semaphore_ptr;
        Anvil::SemaphoreUniquePtr new_wait_semaphore_ptr;

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(m_device_ptr.get());

            new_signal_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr));

            new_signal_semaphore_ptr->set_name_formatted("Signal semaphore [%d]",
                                                         n_semaphore);
        }

        {
            auto create_info_ptr = Anvil::SemaphoreCreateInfo::create(m_device_ptr.get());

            new_wait_semaphore_ptr = Anvil::Semaphore::create(std::move(create_info_ptr));

            new_wait_semaphore_ptr->set_name_formatted("Wait semaphore [%d]",
                                                       n_semaphore);
        }

        m_frame_signal_semaphores.push_back(std::move(new_signal_semaphore_ptr));
        m_frame_wait_semaphores.push_back(std::move(new_wait_semaphore_ptr));
    }
}

void Graphics::init_framebuffers()
{

    for (uint32_t n_swapchain_image = 0;
         n_swapchain_image < m_n_swapchain_images;
         ++n_swapchain_image)
    {
        Anvil::FramebufferUniquePtr framebuffer_ptr;

        {
            auto create_info_ptr = Anvil::FramebufferCreateInfo::create(m_device_ptr.get(),
                                                                        window_with_async_input_ptr -> m_window_ptr -> get_width_at_creation_time(),
                                                                        window_with_async_input_ptr -> m_window_ptr -> get_height_at_creation_time(),
                                                                        1); /* n_layers */

            create_info_ptr->add_attachment(m_swapchain_ptr->get_image_view(n_swapchain_image),
                                            nullptr);

            create_info_ptr->add_attachment(m_depth_image_view_ptr.get(),
                                            nullptr);

            framebuffer_ptr = Anvil::Framebuffer::create(std::move(create_info_ptr));
        }

        framebuffer_ptr->set_name("Main framebuffer");


        m_framebuffers.push_back(
            std::move(framebuffer_ptr)
        );

    }
}

void Graphics::init_renderpasses()
{
    /* We are rendering directly to the swapchain image, so need one renderpass per image */
        Anvil::RenderPassAttachmentID color_attachment_id;
        Anvil::RenderPassAttachmentID depth_attachment_id;

        Anvil::RenderPassCreateInfoUniquePtr renderpass_create_info_ptr(new Anvil::RenderPassCreateInfo(m_device_ptr.get()));

        renderpass_create_info_ptr->add_color_attachment(m_swapchain_ptr->get_create_info_ptr()->get_format(),
                                                            Anvil::SampleCountFlagBits::_1_BIT,
                                                            Anvil::AttachmentLoadOp::CLEAR,
                                                            Anvil::AttachmentStoreOp::STORE,
                                                            Anvil::ImageLayout::UNDEFINED,
                                                            Anvil::ImageLayout::PRESENT_SRC_KHR,
                                                            false, /* may_alias */
                                                            &color_attachment_id);

        renderpass_create_info_ptr->add_depth_stencil_attachment(m_depth_image_ptr->get_create_info_ptr()->get_format(),
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::AttachmentLoadOp::CLEAR,
                                                                    Anvil::AttachmentStoreOp::DONT_CARE,
                                                                    Anvil::AttachmentLoadOp::DONT_CARE,
                                                                    Anvil::AttachmentStoreOp::DONT_CARE,
                                                                    Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                    Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                    false, /* may_alias */
                                                                    &depth_attachment_id);

        /* Define the only subpass we're going to use there */
        renderpass_create_info_ptr->add_subpass(&m_subpass_id);
        renderpass_create_info_ptr->add_subpass_color_attachment(m_subpass_id,
                                                                    Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                    color_attachment_id,
                                                                    0,        /* in_location                      */
                                                                    nullptr); /* in_opt_attachment_resolve_id_ptr */
        renderpass_create_info_ptr->add_subpass_depth_stencil_attachment(m_subpass_id,
                                                                            Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                            depth_attachment_id);


        m_renderpass_ptr = Anvil::RenderPass::create(std::move(renderpass_create_info_ptr),
                                                    m_swapchain_ptr.get());

        m_renderpass_ptr->set_name("Renderpass for swapchain images");
}

void Graphics::init_command_buffers()
{
    const Anvil::DeviceType        device_type = m_device_ptr->get_type();
    auto                           gfx_manager_ptr = m_device_ptr->get_graphics_pipeline_manager();
    const uint32_t                 n_swapchain_images = m_swapchain_ptr->get_create_info_ptr()->get_n_images();
    const uint32_t                 universal_queue_family_index = m_device_ptr->get_universal_queue(0)->get_queue_family_index();

    for (uint32_t n_command_buffer = 0;
         n_command_buffer < m_n_swapchain_images;
         ++n_command_buffer)
    {
        Anvil::PrimaryCommandBufferUniquePtr cmd_buffer_ptr;

        cmd_buffer_ptr = m_device_ptr->get_command_pool_for_queue_family_index(universal_queue_family_index)->alloc_primary_level_command_buffer();

        /* Start recording commands */
        cmd_buffer_ptr->start_recording(false, /* one_time_submit          */
                                        true); /* simultaneous_use_allowed */

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
                                              m_swapchain_ptr->get_image(n_command_buffer),
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
            /* Make sure CPU-written data is flushed before we start rendering */
            Anvil::BufferBarrier buffer_barrier(Anvil::AccessFlagBits::HOST_WRITE_BIT,                 /* in_source_access_mask      */
                                                Anvil::AccessFlagBits::UNIFORM_READ_BIT,               /* in_destination_access_mask */
                                                universal_queue_family_index,                          /* in_src_queue_family_index  */
                                                universal_queue_family_index,                          /* in_dst_queue_family_index  */
                                                m_camera_buffer_ptr.get(),
                                                0,                                                     /* in_offset                  */
                                                sizeof(glm::mat4x4));

            cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::HOST_BIT,
                                                    Anvil::PipelineStageFlagBits::VERTEX_SHADER_BIT,
                                                    Anvil::DependencyFlagBits::NONE,
                                                    0,                                                  /* in_memory_barrier_count        */
                                                    nullptr,                                            /* in_memory_barriers_ptr         */
                                                    1,                                                  /* in_buffer_memory_barrier_count */
                                                    &buffer_barrier,
                                                    0,                                                  /* in_image_memory_barrier_count  */
                                                    nullptr);                                           /* in_image_memory_barriers_ptr   */
        }

        {
            Anvil::DescriptorSet*                ds_ptr(m_dsg_ptr->get_descriptor_set(0));

            VkClearValue  clear_values[2];
            clear_values[0].color.float32[0] = 0.0f;
            clear_values[0].color.float32[1] = 0.0f;
            clear_values[0].color.float32[2] = 0.0f;
            clear_values[0].color.float32[3] = 1.0f;
            clear_values[1].depthStencil.depth = 1.0f;

            VkRect2D  render_area;
            render_area.extent.height = window_with_async_input_ptr->m_window_ptr->get_height_at_creation_time();
            render_area.extent.width = window_with_async_input_ptr->m_window_ptr->get_width_at_creation_time();
            render_area.offset.x = 0;
            render_area.offset.y = 0;

            cmd_buffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                    clear_values,
                                                    m_framebuffers[n_command_buffer].get(),
                                                    render_area,
                                                    m_renderpass_ptr.get(),
                                                    Anvil::SubpassContents::INLINE);

            std::vector<Anvil::DescriptorSet*> descriptor_sets;
            for (size_t i = 0; i < m_dsg_ptr->get_n_descriptor_sets(); i++)
                descriptor_sets.emplace_back(m_dsg_ptr->get_descriptor_set(i));

            sceneNodes_ptr->Draw(PrimitivesSetIndex, cmd_buffer_ptr.get(), descriptor_sets);

            cmd_buffer_ptr->record_end_render_pass();
        }

        cmd_buffer_ptr->stop_recording();

        m_cmd_buffers.push_back(std::move(cmd_buffer_ptr));
    }
}

void Graphics::bind_camera(MovementBaseClass* in_camera)
{
    camera = in_camera;
}

void Graphics::unregister_window_callback(Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr)
{
    window_with_async_input_ptr->m_window_ptr->unregister_from_callbacks(in_callback_id,
                                                                         in_callback_function,
                                                                         in_callback_owner_ptr);
}

void Graphics::register_window_callback(Anvil::CallbackID in_callback_id, Anvil::CallbackFunction in_callback_function, void* in_callback_owner_ptr)
{
    window_with_async_input_ptr->m_window_ptr->register_for_callbacks(in_callback_id,
                                                                      in_callback_function,
                                                                      in_callback_owner_ptr);
}


void Graphics::on_validation_callback(Anvil::DebugMessageSeverityFlags in_severity,
                                      const char*                      in_message_ptr)
{
    if ((in_severity & Anvil::DebugMessageSeverityFlagBits::ERROR_BIT) != 0)
    {
        printf("[!] %s\n",
               in_message_ptr);
    }
}

std::string Graphics::GetFilePathExtension(const std::string &FileName) {
    if (FileName.find_last_of(".") != std::string::npos)
        return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
}
