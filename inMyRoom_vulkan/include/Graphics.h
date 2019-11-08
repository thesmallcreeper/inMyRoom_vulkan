#pragma once

#include <memory>
#include <thread>
#include <future>
#include <fstream>
#include <utility>

#include "configuru.hpp"

#include "tiny_gltf.h"

#include "misc/swapchain_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/semaphore_create_info.h"
#include "misc/fence_create_info.h"
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
#include "wrappers/fence.h"

#include "WindowWithAsyncInput.h"
#include "CameraBaseClass.h"
#include "ViewportFrustum.h"

#include "ImagesAboutOfTextures.h"
#include "TexturesOfMaterials.h"
#include "MaterialsOfPrimitives.h"
#include "ShadersSetsFamiliesCache.h"
#include "PipelinesFactory.h"
#include "NodesOfScene.h"
#include "PrimitivesOfMeshes.h"
#include "MipmapsGenerator.h"
#include "MeshesOfNodes.h"
#include "Drawer.h"

class Graphics
{
public:
    Graphics(configuru::Config& in_cfgFile, Anvil::BaseDevice* in_device_ptr, Anvil::Swapchain* in_swapchain_ptr,
             uint32_t windowWidth, uint32_t windowHeight, uint32_t swapchainImagesCount);
    ~Graphics();

    void BindCamera(CameraBaseClass* in_camera);

    void DrawFrame();


private:
    std::string GetFilePathFolder(const std::string& in_fileName);
    std::string GetFilePathExtension(const std::string& in_fileName);

    void LoadScene();

    void InitCameraBuffers();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitScene();
    void InitCameraDsg();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPipelinesFactory();
    void InitShadersSetsFamiliesCache();

    void RecordCommandBuffer(uint32_t swapchainImageIndex);

private:

    tinygltf::Model model;

    std::unique_ptr<TexturesOfMaterials> texturesOfMaterials_uptr;
    std::unique_ptr<MaterialsOfPrimitives> materialsOfPrimitives_uptr;
    std::unique_ptr<ShadersSetsFamiliesCache> shadersSetsFamiliesCache_uptr;
    std::unique_ptr<PipelinesFactory> pipelinesFactory_uptr;
    std::unique_ptr<NodesOfScene> nodesOfScene_uptr;
    std::unique_ptr<PrimitivesOfMeshes> primitivesOfMeshes_uptr;
    std::unique_ptr<MeshesOfNodes> meshesOfNodes_uptr;

    CameraBaseClass* camera_ptr;
    ViewportFrustum cameraFrustum;
    ViewportFrustum cullingFrustum;

    const uint32_t              swapchainImagesCount;
    const uint32_t              windowWidth;
    const uint32_t              windowHeight;

    std::vector<Anvil::BufferUniquePtr>          cameraBuffer_uptrs;
    std::vector<Anvil::BufferUniquePtr>          perspectiveBuffer_uptrs;

    Anvil::DescriptorSetGroupUniquePtr	cameraDescriptorSetGroup_uptr;

    Anvil::ImageUniquePtr           depthImage_uptr;
    Anvil::ImageViewUniquePtr       depthImageView_uptr;

    std::vector<Anvil::SemaphoreUniquePtr> frameSignalSemaphores_uptrs;
    std::vector<Anvil::SemaphoreUniquePtr> frameWaitSemaphores_uptrs;

    std::vector<Anvil::FramebufferUniquePtr>        framebuffers_uptrs;

    Anvil::RenderPassUniquePtr                      renderpass_uptr;

    Anvil::FenceUniquePtr                           fence_last_submit_uptr;

    uint32_t                      lastSemaphoreUsed;

    Anvil::SubPassID              textureSubpassID;

    size_t zprepassPassSetIndex;
    size_t texturePassSetIndex;
 
    std::vector<Anvil::PrimaryCommandBufferUniquePtr> cmdBuffers_uptrs;

    const configuru::Config& cfgFile;

    Anvil::BaseDevice* const     device_ptr;
    Anvil::Swapchain* const      swapchain_ptr;
};