#pragma once

#include "configuru.hpp"

#include "vulkan/vulkan.hpp"
#include "tiny_gltf.h"

#include "WindowWithAsyncInput.h"

#include "Graphics/PipelinesFactory.h"
#include "Graphics/VulkanInit.h"

#include "ECS/GeneralComponents/AnimationActorComp.h"
#include "ECS/GeneralComponents/CameraComp.h"
#include "ECS/GeneralComponents/ModelDrawComp.h"
#include "ECS/GeneralComponents/DynamicMeshComp.h"
#include "ECS/GeneralComponents/LightComp.h"

#include "Graphics/ShadersSetsFamiliesCache.h"
#include "Graphics/DynamicMeshes.h"
#include "Graphics/Meshes/AnimationsDataOfNodes.h"
#include "Graphics/Meshes/TexturesOfMaterials.h"
#include "Graphics/Meshes/MaterialsOfPrimitives.h"
#include "Graphics/Meshes/MeshesOfNodes.h"
#include "Graphics/Meshes/PrimitivesOfMeshes.h"
#include "Graphics/Meshes/SkinsOfMeshes.h"
#include "Graphics/Renderers/Renderer.h"

class Engine;       // Forward declaration

class Graphics
{
public:
    Graphics(Engine* engine_ptr, configuru::Config& cfgFile, vk::Device device, vma::Allocator vma_allocator);
    ~Graphics();

    MeshesOfNodes* GetMeshesOfNodesPtr() const {return meshesOfNodes_uptr.get();}
    PrimitivesOfMeshes* GetPrimitivesOfMeshes() const {return primitivesOfMeshes_uptr.get();}
    SkinsOfMeshes* GetSkinsOfMeshesPtr() const {return skinsOfMeshes_uptr.get();}
    AnimationsDataOfNodes* GetAnimationsDataOfNodesPtr() const {return animationsDataOfNodes_uptr.get();}
    MaterialsOfPrimitives* GetMaterialsOfPrimitives() const {return materialsOfPrimitives_uptr.get();}
    TexturesOfMaterials* GetTexturesOfMaterials() const {return texturesOfMaterials_uptr.get();}
    DynamicMeshes* GetDynamicMeshes() const {return dynamicMeshes_uptr.get();}
    Lights* GetLights() const {return lights_uptr.get();}
    PipelinesFactory* GetPipelineFactory() const {return pipelinesFactory_uptr.get();}
    ShadersSetsFamiliesCache* GetShadersSetsFamiliesCache() const {return shadersSetsFamiliesCache_uptr.get();}

    vk::DescriptorSetLayout GetCameraDescriptionSetLayout() {return cameraDescriptorSetLayout;}
    vk::DescriptorSet GetCameraDescriptionSet(size_t index) {return cameraDescriptorSets[index];}

    vk::DescriptorSetLayout GetMatricesDescriptionSetLayout() {return matricesDescriptorSetLayout;}
    vk::DescriptorSet GetMatricesDescriptionSet(size_t index) {return matricesDescriptorSets[index];}

    vk::SwapchainCreateInfoKHR GetSwapchainCreateInfo() const;
    std::vector<vk::ImageView> GetSwapchainImageViews() const;
    vk::SwapchainKHR GetSwapchain() const;
    QueuesList GetQueuesList() const;

    size_t GetSubgroupSize() const;

    size_t GetMaxInstancesCount() const {return maxInstances;}

    void LoadModel(const tinygltf::Model& in_model, std::string in_model_images_folder);
    void EndModelsLoad();

    void DrawFrame();

    void WriteCameraMarticesBuffers(ViewportFrustum viewport,
                                    const std::vector<ModelMatrices>& model_matrices,
                                    const std::vector<DrawInfo>& draw_infos,
                                    size_t buffer_index);

    void ToggleViewportFreeze();
    float GetDeltaTimeSeconds() const;

    // TODO: Get the f out
    void ToggleCullingDebugging();

private:
    void InitBuffers();
    void InitDescriptors();
    void InitPipelinesFactory();
    void InitShadersSetsFamiliesCache();
    void InitMeshesTree();
    void InitGraphicsComponents();
    void InitDynamicMeshes();
    void InitLights();
    void InitRenderer();

private:
    std::pair<vk::Queue, uint32_t> graphicsQueue;
    std::pair<vk::Queue, uint32_t> computeQueue;

    std::unique_ptr<RendererBase> renderer_uptr;

    vk::Buffer              cameraBuffer;
    vma::Allocation         cameraAllocation;
    vma::AllocationInfo     cameraAllocInfo;

    vk::Buffer              matricesBuffer;
    vma::Allocation         matricesAllocation;
    vma::AllocationInfo     matricesAllocInfo;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       cameraDescriptorSets[3];
    vk::DescriptorSetLayout cameraDescriptorSetLayout;
    vk::DescriptorSet       matricesDescriptorSets[3];
    vk::DescriptorSetLayout matricesDescriptorSetLayout;

    std::unique_ptr<AnimationActorComp> animationActorComp_uptr;
    std::unique_ptr<CameraComp> cameraComp_uptr;
    std::unique_ptr<ModelDrawComp> modelDrawComp_uptr;
    std::unique_ptr<DynamicMeshComp> dynamicMeshComp_uptr;
    std::unique_ptr<LightComp> lightComp_uptr;

    std::unique_ptr<AnimationsDataOfNodes> animationsDataOfNodes_uptr;
    std::unique_ptr<TexturesOfMaterials> texturesOfMaterials_uptr;
    std::unique_ptr<MaterialsOfPrimitives> materialsOfPrimitives_uptr;
    std::unique_ptr<PrimitivesOfMeshes> primitivesOfMeshes_uptr;
    std::unique_ptr<SkinsOfMeshes> skinsOfMeshes_uptr;
    std::unique_ptr<MeshesOfNodes> meshesOfNodes_uptr;

    std::unique_ptr<DynamicMeshes> dynamicMeshes_uptr;
    std::unique_ptr<Lights> lights_uptr;

    std::unique_ptr<ShadersSetsFamiliesCache> shadersSetsFamiliesCache_uptr;
    std::unique_ptr<PipelinesFactory> pipelinesFactory_uptr;

    vk::Device              device;
    vma::Allocator          vma_allocator;

    Engine* const           engine_ptr;
    configuru::Config&      cfgFile;

    const size_t maxInstances = std::numeric_limits<uint16_t>::max();
};