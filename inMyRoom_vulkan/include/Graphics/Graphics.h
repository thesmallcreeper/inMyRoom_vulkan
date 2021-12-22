#pragma once

#include "configuru.hpp"

#include "vulkan/vulkan.hpp"
#include "tiny_gltf.h"

#include "WindowWithAsyncInput.h"

#include "Graphics/PipelinesFactory.h"

#include "ECS/GeneralComponents/AnimationActorComp.h"
#include "ECS/GeneralComponents/CameraComp.h"
#include "ECS/GeneralComponents/ModelDrawComp.h"
#include "ECS/GeneralComponents/DynamicMeshComp.h"

#include "Geometry/ViewportFrustum.h"

#include "Graphics/ShadersSetsFamiliesCache.h"
#include "Graphics/DynamicMeshes.h"
#include "Graphics/Meshes/AnimationsDataOfNodes.h"
#include "Graphics/Meshes/TexturesOfMaterials.h"
#include "Graphics/Meshes/MaterialsOfPrimitives.h"
#include "Graphics/Meshes/MeshesOfNodes.h"
#include "Graphics/Meshes/PrimitivesOfMeshes.h"
#include "Graphics/Meshes/SkinsOfMeshes.h"


class Engine;       // Forward declaration

class Graphics
{
public:
    Graphics(Engine* engine_ptr, configuru::Config& cfgFile, vk::Device device, vma::Allocator vma_allocator);
    ~Graphics();

    MeshesOfNodes* GetMeshesOfNodesPtr() const {return meshesOfNodes_uptr.get();};
    PrimitivesOfMeshes* GetPrimitivesOfMeshes() const {return primitivesOfMeshes_uptr.get();}
    SkinsOfMeshes* GetSkinsOfMeshesPtr() const {return skinsOfMeshes_uptr.get();};
    AnimationsDataOfNodes* GetAnimationsDataOfNodesPtr() {return animationsDataOfNodes_uptr.get();};

    PipelinesFactory* GetPipelineFactory() const {return pipelinesFactory_uptr.get();}
    ShadersSetsFamiliesCache* GetShadersSetsFamiliesCache() const {return shadersSetsFamiliesCache_uptr.get();}

    vk::DescriptorSetLayout GetMatricesDescriptionSetLayout() {return matricesDescriptorSetLayout;}
    vk::DescriptorSet GetMatricesDescriptionSet() {return matricesDescriptorSets[frameCount % 2];}

    size_t GetMaxInstancesCount() const {return maxInstances;}

    void LoadModel(const tinygltf::Model& in_model, std::string in_model_images_folder);
    void EndModelsLoad();

    void DrawFrame();

    void ToggleCullingDebugging();

private:
    void InitBuffers();
    void InitDescriptors();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPipelinesFactory();
    void InitShadersSetsFamiliesCache();
    void InitMeshesTree();
    void InitGraphicsComponents();
    void InitDynamicMeshes();

    void RecordCommandBuffer(vk::CommandBuffer command_buffer,
                             uint32_t buffer_index,
                             uint32_t swapchain_index,
                             std::vector<DrawInfo>& draw_infos,
                             const FrustumCulling& frustum_culling);

private:
    std::pair<vk::Queue, uint32_t> graphicsQueue;
    size_t                  frameCount = 0;

    vk::Buffer              cameraBuffer;
    vma::Allocation         cameraAllocation;
    vma::AllocationInfo     cameraAllocInfo;

    vk::Buffer              matricesBuffer;
    vma::Allocation         matricesAllocation;
    vma::AllocationInfo     matricesAllocInfo;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       cameraDescriptorSets[2];
    vk::DescriptorSetLayout cameraDescriptorSetLayout;
    vk::DescriptorSet       matricesDescriptorSets[2];
    vk::DescriptorSetLayout matricesDescriptorSetLayout;

    vk::Image               depthImage;
    vma::Allocation         depthAllocation;
    vk::ImageCreateInfo     depthImageCreateInfo;
    vk::ImageView           depthImageView;

    vk::RenderPass          renderpass;

    std::vector<vk::Framebuffer> framebuffers;

    vk::Semaphore           imageAvailableSemaphore;
    vk::Semaphore           renderFinishSemaphore;

    vk::CommandPool         commandPool;
    vk::CommandBuffer       commandBuffer;

    std::vector<vk::Pipeline>       primitivesPipelines;
    std::vector<vk::PipelineLayout> primitivesPipelineLayouts;

    std::unique_ptr<AnimationActorComp> animationActorComp_uptr;
    std::unique_ptr<CameraComp> cameraComp_uptr;
    std::unique_ptr<ModelDrawComp> modelDrawComp_uptr;
    std::unique_ptr<DynamicMeshComp> dynamicMeshComp_uptr;

    std::unique_ptr<AnimationsDataOfNodes> animationsDataOfNodes_uptr;
    std::unique_ptr<TexturesOfMaterials> texturesOfMaterials_uptr;
    std::unique_ptr<MaterialsOfPrimitives> materialsOfPrimitives_uptr;
    std::unique_ptr<PrimitivesOfMeshes> primitivesOfMeshes_uptr;
    std::unique_ptr<SkinsOfMeshes> skinsOfMeshes_uptr;
    std::unique_ptr<MeshesOfNodes> meshesOfNodes_uptr;
    std::unique_ptr<DynamicMeshes> dynamicMeshes_uptr;

    std::unique_ptr<ShadersSetsFamiliesCache> shadersSetsFamiliesCache_uptr;
    std::unique_ptr<PipelinesFactory> pipelinesFactory_uptr;

    vk::Device              device;
    vma::Allocator          vma_allocator;

    Engine* const           engine_ptr;
    configuru::Config&      cfgFile;

    const size_t maxInstances = std::numeric_limits<uint16_t>::max();
};