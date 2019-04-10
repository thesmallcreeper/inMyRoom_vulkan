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
    void InitSpacialDsg();
    void InitSemaphores();
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

    const uint32_t              swapchainImagesCount;
    const uint32_t              windowWidth;
    const uint32_t              windowHeight;

    Anvil::BufferUniquePtr          cameraBuffer_uptr;
    Anvil::BufferUniquePtr          perspectiveBuffer_uptr;

    Anvil::DescriptorSetGroupUniquePtr	spacialDSG_uptr;

    Anvil::ImageUniquePtr           depthImage_uptr;
    Anvil::ImageViewUniquePtr       depthImageView_uptr;

    std::vector<Anvil::SemaphoreUniquePtr> frameSignalSemaphores_uptrs;
    std::vector<Anvil::SemaphoreUniquePtr> frameWaitSemaphores_uptrs;

    std::vector<Anvil::FramebufferUniquePtr>        framebuffers_uptrs;

    Anvil::RenderPassUniquePtr                      renderpass_uptr;

    uint32_t                      lastSemaphoreUsed;

    Anvil::SubPassID              colorSubpassID;

    size_t generalPrimitivesSetIndex;

    std::vector<Anvil::PrimaryCommandBufferUniquePtr> cmdBuffers_uptrs;

    const configuru::Config& cfgFile;
};
