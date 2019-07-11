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
#include "ViewportFrustum.h"

#include "TexturesImagesUsage.h"
#include "MaterialsTextures.h"
#include "PrimitivesMaterials.h"
#include "PrimitivesShaders.h"
#include "PrimitivesPipelines.h"
#include "SceneNodes.h"
#include "MeshesPrimitives.h"
#include "NodesMeshes.h"
#include "Drawer.h"

class Graphics
{
public:
    Graphics(configuru::Config& in_cfgFile, Anvil::BaseDevice* in_device_ptr, Anvil::Swapchain* in_swapchain_ptr,
             uint32_t windowWidth, uint32_t windowHeight, uint32_t swapchainImagesCount);
    ~Graphics();

    void BindCamera        (CameraBaseClass* in_camera);

    void DrawFrame         ();


private:
    std::string GetFilePathExtension(const std::string &FileName);

    void LoadScene();

    void InitCameraBuffers();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitScene();
    void InitCameraDsg();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();

    void RecordCommandBuffer(uint32_t swapchainImageIndex);

private:
    Anvil::BaseDevice* const     device_ptr;
    Anvil::Swapchain* const      swapchain_ptr;

    tinygltf::Model model;

    std::unique_ptr<TexturesImagesUsage> texturesImagesUsage_uptr;
    std::unique_ptr<MaterialsTextures> materialsTextures_uptr;
    std::unique_ptr<PrimitivesMaterials> primitivesMaterials_uptr;
    std::unique_ptr<PrimitivesShaders> primitivesShaders_uptr;
    std::unique_ptr<PrimitivesPipelines> primitivesPipelines_uptr;
    std::unique_ptr<SceneNodes> sceneNodes_uptr;
    std::unique_ptr<MeshesPrimitives> meshesPrimitives_uptr;
    std::unique_ptr<NodesMeshes> nodesMeshes_uptr;

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

    Anvil::SubPassID              zprepassSubpassID;
    Anvil::SubPassID              textureSubpassID;

    size_t zprepassPassSetIndex;
    size_t texturePassSetIndex;
 
    std::vector<Anvil::PrimaryCommandBufferUniquePtr> cmdBuffers_uptrs;

    const configuru::Config& cfgFile;
};
