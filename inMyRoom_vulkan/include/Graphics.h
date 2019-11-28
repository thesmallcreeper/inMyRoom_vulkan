#pragma once

#include <memory>
#include <thread>
#include <future>
#include <fstream>
#include <utility>
#include <cassert>

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

#include "ECS/GeneralComponents/ModelDrawComp.h"
#include "Geometry/ViewportFrustum.h"

#include "WindowWithAsyncInput.h"
#include "CameraBaseClass.h"

#include "PipelinesFactory.h"
#include "MipmapsGenerator.h"
#include "Drawer.h"

#include "Meshes/ImagesAboutOfTextures.h"
#include "Meshes/TexturesOfMaterials.h"
#include "Meshes/MaterialsOfPrimitives.h"
#include "Meshes/MeshesOfNodes.h"
#include "Meshes/PrimitivesOfMeshes.h"
#include "ShadersSetsFamiliesCache.h"

class Engine;       // Forward declaration

class Graphics
{
public:
    Graphics(Engine* in_engine_ptr, configuru::Config& in_cfgFile, Anvil::BaseDevice* in_device_ptr, Anvil::Swapchain* in_swapchain_ptr,
             uint32_t windowWidth, uint32_t windowHeight, uint32_t swapchainImagesCount);
    ~Graphics();

    void LoadModel(const tinygltf::Model& in_model, std::string in_model_images_folder);
    void EndModelsLoad();

    void BindCamera(CameraBaseClass* in_camera);

    void DrawFrame();

    MeshesOfNodes* GetMeshesOfNodesPtr();

private:


    void InitCameraBuffers();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitCameraDsg();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPipelinesFactory();
    void InitShadersSetsFamiliesCache();
    void InitMeshesTree();
    void InitGraphicsComponents();

    void RecordCommandBuffer(uint32_t swapchainImageIndex);

private:
    std::unique_ptr<ModelDrawComp> modelDrawComp_uptr;

    std::unique_ptr<MipmapsGenerator> mipmapsGenerator_uptr;
    std::unique_ptr<ImagesAboutOfTextures> imagesAboutOfTextures_uptr;
    std::unique_ptr<TexturesOfMaterials> texturesOfMaterials_uptr;
    std::unique_ptr<MaterialsOfPrimitives> materialsOfPrimitives_uptr;
    std::unique_ptr<PrimitivesOfMeshes> primitivesOfMeshes_uptr;
    std::unique_ptr<MeshesOfNodes> meshesOfNodes_uptr;

    std::unique_ptr<ShadersSetsFamiliesCache> shadersSetsFamiliesCache_uptr;
    std::unique_ptr<PipelinesFactory> pipelinesFactory_uptr;

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

    Engine* const          engine_ptr;
};