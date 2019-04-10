#define GLM_FORECE_DEPTH_ZERO_TO_ONE

#include "Graphics.h"

#include "glm/mat4x4.hpp"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"

#include "misc/swapchain_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/semaphore_create_info.h"
#include "misc/framebuffer_create_info.h"
#include "misc/render_pass_create_info.h"
#include "wrappers/rendering_surface.h"
#include "wrappers/buffer.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/framebuffer.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/swapchain.h"
#include "wrappers/render_pass.h"
#include "wrappers/semaphore.h"


Graphics::Graphics(configuru::Config& in_cfgFile, Anvil::BaseDevice* in_device_ptr, Anvil::Swapchain* in_swapchain_ptr,
                   uint32_t windowWidth, uint32_t windowHeight, uint32_t swapchainImagesCount)
    :
    cfgFile(in_cfgFile),
    swapchainImagesCount(swapchainImagesCount),
    lastSemaphoreUsed(0),
    device_ptr(in_device_ptr),
    swapchain_ptr(in_swapchain_ptr),
    windowWidth(windowWidth),
    windowHeight(windowHeight)
{
    LoadScene();

    InitCameraBuffers();
    InitImages();
    InitFramebuffers();
    InitRenderpasses();
    InitSemaphores();
    InitCommandBuffers();

    InitScene();
}

Graphics::~Graphics()
{

    frameSignalSemaphores_uptrs.clear();
    frameWaitSemaphores_uptrs.clear();
    framebuffers_uptrs.clear();

    renderpass_uptr.reset();
    depthImage_uptr.reset();
    depthImageView_uptr.reset();

    nodesMeshes_uptr.reset();
    materialsTextures_uptr.reset();
    texturesImagesUsage_uptr.reset();
    primitivesMaterials_uptr.reset();
    primitivesShaders_uptr.reset();
    primitivesPipelines_uptr.reset();
    meshesPrimitives_uptr.reset();

    spacialDSG_uptr.reset();

    sceneNodes_uptr.reset();

    cameraBuffer_uptr.reset();
    perspectiveBuffer_uptr.reset();
}

void Graphics::DrawFrame()
{
    Anvil::Semaphore*               curr_frame_signal_semaphore_ptr = nullptr;
    Anvil::Semaphore*               curr_frame_wait_semaphore_ptr = nullptr;
    static uint32_t                 n_frames_rendered = 0;
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

    Anvil::Vulkan::vkDeviceWaitIdle(device_ptr->get_device_vk());

    camera_ptr->RefreshPublicVectors();
    glm::vec3 cameraPosition = camera_ptr->publicPosition;
    glm::vec3 cameraLookingDirection = camera_ptr->publicLookingDirection;
    glm::vec3 cameraUp = camera_ptr->upVector;

    glm::mat4x4 camera_matrix = glm::lookAt(cameraPosition, cameraPosition + cameraLookingDirection, cameraUp);

    cameraBuffer_uptr->write(0,
                             sizeof(glm::mat4x4),
                             &camera_matrix,
                             present_queue_ptr);

    RecordCommandBuffer(n_swapchain_image);

    present_queue_ptr->submit(
        Anvil::SubmitInfo::create(cmdBuffers_uptrs[n_swapchain_image].get(),
                                  1,
                                  &curr_frame_signal_semaphore_ptr,
                                  1,
                                  &curr_frame_wait_semaphore_ptr,
                                  &wait_stage_mask,
                                  false /* should_block */)
    );

    {
        Anvil::SwapchainOperationErrorCode present_result = Anvil::SwapchainOperationErrorCode::DEVICE_LOST;
        present_queue_ptr->present(swapchain_ptr,
                                   n_swapchain_image,
                                   1, /* n_wait_semaphores */
                                   &curr_frame_signal_semaphore_ptr,
                                   &present_result);

        anvil_assert(present_result == Anvil::SwapchainOperationErrorCode::SUCCESS);
    }
}

void Graphics::RecordCommandBuffer(uint32_t swapchainImageIndex)
{
    const uint32_t                 universal_queue_family_index = device_ptr->get_universal_queue(0)->get_queue_family_index();

    Anvil::PrimaryCommandBuffer* cmd_buffer_ptr = cmdBuffers_uptrs[swapchainImageIndex].get();

    /* Reset command buffer*/
    cmd_buffer_ptr->reset(false);

    /* Start recording commands */
    cmd_buffer_ptr->start_recording(true,  /* one_time_submit          */
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
            /* Make sure CPU-written data is flushed before we start rendering */
            Anvil::BufferBarrier buffer_barrier(Anvil::AccessFlagBits::MEMORY_WRITE_BIT,               /* in_source_access_mask      */
                                                Anvil::AccessFlagBits::UNIFORM_READ_BIT,               /* in_destination_access_mask */
                                                universal_queue_family_index,                          /* in_src_queue_family_index  */
                                                universal_queue_family_index,                          /* in_dst_queue_family_index  */
                                                cameraBuffer_uptr.get(),
                                                0,                                                     /* in_offset                  */
                                                sizeof(glm::mat4x4));

            cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TRANSFER_BIT,
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
        Anvil::DescriptorSet*                ds_ptr(spacialDSG_uptr->get_descriptor_set(0));

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

        std::vector<Anvil::DescriptorSet*> descriptor_sets;
        for (size_t i = 0; i < spacialDSG_uptr->get_n_descriptor_sets(); i++)
            descriptor_sets.emplace_back(spacialDSG_uptr->get_descriptor_set(i));

        sceneNodes_uptr->Draw(generalPrimitivesSetIndex, cmd_buffer_ptr, descriptor_sets);

        cmd_buffer_ptr->record_end_render_pass();
    }

    cmd_buffer_ptr->stop_recording();
}

void Graphics::BindCamera(CameraBaseClass* in_camera)
{
    camera_ptr = in_camera;
}


void Graphics::LoadScene()
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


void Graphics::InitCameraBuffers()
{
    Anvil::MemoryAllocatorUniquePtr   allocator_ptr;

    const Anvil::DeviceType           device_type(device_ptr->get_type());

    const Anvil::MemoryFeatureFlags   required_feature_flags = Anvil::MemoryFeatureFlagBits::NONE;

    allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                        sizeof(glm::mat4),
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

        cameraBuffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

        cameraBuffer_uptr->set_name("Camera matrix buffer");
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                        sizeof(glm::mat4),
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        Anvil::BufferCreateFlagBits::NONE,
                                                                        Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

        perspectiveBuffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

        perspectiveBuffer_uptr->set_name("Camera matrix buffer");
    }

    allocator_ptr->add_buffer(cameraBuffer_uptr.get(),
                              required_feature_flags);

    allocator_ptr->add_buffer(perspectiveBuffer_uptr.get(),
                              required_feature_flags);

    // Camera buffer is being updated every frame

    glm::mat4x4 perspective_matrix = glm::perspective(glm::radians(cfgFile["graphicsSettings"]["FOV"].as_float()),
                                                      static_cast<float>(windowWidth) / static_cast<float>(windowHeight),
                                                      cfgFile["graphicsSettings"]["nearPlaneDistance"].as_float(),
                                                      cfgFile["graphicsSettings"]["farPlaneDistance"].as_float()) *
                                     glm::mat4(1.0f, 0.0f, 0.0f, 0.0f,                                                                                                                                                                                                                           
                                               0.0f, -1.0f, 0.0f, 0.0f,
                                               0.0f, 0.0f, 1.0f, 0.0f,
                                               0.0f, 0.0f, 0.0f, 1.0f);

    perspectiveBuffer_uptr->write(0,
                                    sizeof(glm::mat4x4),
                                    &perspective_matrix);

}

void Graphics::InitScene()
{
    const tinygltf::Scene &scene = model.scenes[0];

    texturesImagesUsage_uptr = std::make_unique<TexturesImagesUsage>(model);
    materialsTextures_uptr = std::make_unique <MaterialsTextures>(model, cfgFile["sceneInput"]["imagesFolder"].as_string(), cfgFile["graphicsSettings"]["useMipmaps"].as_bool(), texturesImagesUsage_uptr.get(), device_ptr);
    primitivesMaterials_uptr = std::make_unique<PrimitivesMaterials>(model, materialsTextures_uptr.get(), device_ptr);
    {
        std::vector<ShaderSetFamilyInitInfo> shaderSetsInitInfos;
        ShaderSetFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "General Mesh";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "generalMesh_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "generalMesh_glsl.vert";

        shaderSetsInitInfos.emplace_back(this_shaderSetInitInfo);

        primitivesShaders_uptr = std::make_unique<PrimitivesShaders>(shaderSetsInitInfos, device_ptr);
    }

    primitivesPipelines_uptr = std::make_unique<PrimitivesPipelines>(device_ptr);
    
    sceneNodes_uptr = std::make_unique<SceneNodes>(model, scene, device_ptr);

    InitSpacialDsg();

    meshesPrimitives_uptr = std::make_unique<MeshesPrimitives>(primitivesPipelines_uptr.get(), primitivesShaders_uptr.get(), primitivesMaterials_uptr.get(), device_ptr);
    nodesMeshes_uptr = std::make_unique<NodesMeshes>(model, meshesPrimitives_uptr.get(), device_ptr);

    meshesPrimitives_uptr->FlashBuffersToDevice();
    {
        ShadersSpecs this_shaders_specs;
        this_shaders_specs.shadersSetFamilyName = "General Mesh";
        this_shaders_specs.definitionValuePairs.emplace_back(std::make_pair("N_MESHIDS", sceneNodes_uptr->globalTRSmatrixesCount));
        generalPrimitivesSetIndex = meshesPrimitives_uptr->InitPrimitivesSet(this_shaders_specs, true, spacialDSG_uptr->get_descriptor_set_create_info(), renderpass_uptr.get(), colorSubpassID);
    }

    sceneNodes_uptr->BindSceneMeshes(nodesMeshes_uptr.get());
}


void Graphics::InitSpacialDsg()
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


    new_dsg_ptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                    { new_dsg_create_info_ptr },
                                                    false); /* in_releaseable_sets */

    new_dsg_ptr->set_binding_item(0, /* n_set         */
                                  0, /* binding_index */
                                  Anvil::DescriptorSet::StorageBufferBindingElement(sceneNodes_uptr->globalTRSmatrixesBuffer_uptr.get()));

    new_dsg_ptr->set_binding_item(1, /* n_set         */
                                  0, /* binding_index */
                                  Anvil::DescriptorSet::UniformBufferBindingElement(perspectiveBuffer_uptr.get()));

    new_dsg_ptr->set_binding_item(1, /* n_set         */
                                  1, /* binding_index */
                                  Anvil::DescriptorSet::UniformBufferBindingElement(cameraBuffer_uptr.get()));

    spacialDSG_uptr = std::move(new_dsg_ptr);
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

void Graphics::InitSemaphores()
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

        /* Define the only subpass we're going to use there */
        renderpass_create_info_ptr->add_subpass(&colorSubpassID);
        renderpass_create_info_ptr->add_subpass_color_attachment(colorSubpassID,
                                                                    Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                    color_attachment_id,
                                                                    0,        /* in_location                      */
                                                                    nullptr); /* in_opt_attachment_resolve_id_ptr */
        renderpass_create_info_ptr->add_subpass_depth_stencil_attachment(colorSubpassID,
                                                                            Anvil::ImageLayout::DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
                                                                            depth_attachment_id);


        renderpass_uptr = Anvil::RenderPass::create(std::move(renderpass_create_info_ptr),
                                                    swapchain_ptr);

        renderpass_uptr->set_name("Renderpass for swapchain images");
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
std::string Graphics::GetFilePathExtension(const std::string &FileName) 
{
    if (FileName.find_last_of(".") != std::string::npos)
        return FileName.substr(FileName.find_last_of(".") + 1);
    return "";
}