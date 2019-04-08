#pragma once

#include <memory>
#include <thread>
#include <future>
#include <fstream>
#include <utility>

#include "configuru.hpp"


#include "tiny_gltf.h"

#include "WindowWithAsyncInput.h"
#include "CameraBaseClass.h"

#include "TexturesImagesUsage.h"
#include "MaterialsTextures.h"
#include "PrimitivesMaterials.h"
#include "PrimitivesShaders.h"
#include "PrimitivesPipelines.h"
#include "SceneNodes.h"
#include "MeshesPrimitives.h"
#include "NodesMeshes.h"


class Graphics
{
public:
    Graphics(configuru::Config& in_cfgFile, Anvil::BaseDevice* in_device_ptr, Anvil::Swapchain* in_swapchain_ptr,
             uint32_t windowWidth, uint32_t windowHeight, uint32_t swapchainImagesCount);
    ~Graphics();

    void bind_camera        (CameraBaseClass* in_camera);

    void draw_frame         ();


private:
    std::string GetFilePathExtension(const std::string &FileName);

    void load_scene();

    void init_camera_buffers();
    void init_images();
    void init_framebuffers();
    void init_renderpasses();
    void init_scene();
    void init_spacial_dsg();
    void init_semaphores();

    void init_command_buffers();

private:
    Anvil::BaseDevice* const     m_device_ptr;
    Anvil::Swapchain* const      m_swapchain_ptr;

    tinygltf::Model model;

    std::unique_ptr<TexturesImagesUsage> texturesImagesUsage_ptr;
    std::unique_ptr<MaterialsTextures> materialsTextures_ptr;
    std::unique_ptr<PrimitivesMaterials> primitivesMaterials_ptr;
    std::unique_ptr<PrimitivesShaders> primitivesShaders_ptr;
    std::unique_ptr<PrimitivesPipelines> primitivesPipelines_ptr;
    std::unique_ptr<SceneNodes> sceneNodes_ptr;
    std::unique_ptr<MeshesPrimitives> meshesPrimitives_ptr;
    std::unique_ptr<NodesMeshes> nodesMeshes_ptr;

    CameraBaseClass* camera_ptr;

    const uint32_t              m_n_swapchain_images;
    const uint32_t              windowWidth;
    const uint32_t              windowHeight;

    Anvil::BufferUniquePtr          m_camera_buffer_ptr;
    Anvil::BufferUniquePtr          m_perspective_buffer_ptr;

    Anvil::DescriptorSetGroupUniquePtr	m_dsg_ptr;

    Anvil::ImageUniquePtr           m_depth_image_ptr;
    Anvil::ImageViewUniquePtr       m_depth_image_view_ptr;

    std::vector<Anvil::SemaphoreUniquePtr> m_frame_signal_semaphores;
    std::vector<Anvil::SemaphoreUniquePtr> m_frame_wait_semaphores;

    std::vector<Anvil::FramebufferUniquePtr>        m_framebuffers;

    Anvil::RenderPassUniquePtr                      m_renderpass_ptr;

    std::vector<Anvil::PrimaryCommandBufferUniquePtr> m_cmd_buffers;
    uint32_t                      m_n_last_semaphore_used;

    Anvil::SubPassID              m_subpass_id;

    size_t PrimitivesSetIndex;

    const configuru::Config& cfgFile;
};
