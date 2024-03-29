#include "Graphics/Graphics.h"

#include "Geometry/FrustumCulling.h"
#include "Engine.h"

#include "Graphics/Renderers/OfflineRenderer.h"
#include "Graphics/Renderers/RealtimeRenderer.h"

#include <utility>
#include <iostream>
#include <cassert>
#include <array>

Graphics::Graphics(Engine* in_engine_ptr, configuru::Config& in_cfgFile, vk::Device in_device, vma::Allocator in_vma_allocator)
    :engine_ptr(in_engine_ptr),
     cfgFile(in_cfgFile),
     device(in_device),
     vma_allocator(in_vma_allocator)
{
    graphicsQueue = engine_ptr->GetQueuesList().graphicsQueues[0];
    computeQueue = engine_ptr->GetQueuesList().dedicatedComputeQueues[0];

    std::cout << "Initializing camera buffers\n";
    InitBuffers();
    std::cout << "Initializing camera descriptor set\n";
    InitDescriptors();
    std::cout << "Initializing pipelines factory\n";
    InitPipelinesFactory();
    std::cout << "Initializing shaders sets families cache\n";
    InitShadersSetsFamiliesCache();
    std::cout << "Initializing meshes tree\n";
    InitMeshesTree();
    std::cout << "Initializing dynamic meshes\n";
    InitDynamicMeshes();
    std::cout << "Initializing lights\n";
    InitLights();
    std::cout << "Initializing graphics oriented components\n";
    InitGraphicsComponents();
}

Graphics::~Graphics()
{
    renderer_uptr.reset();

    device.waitIdle();

    device.destroy(descriptorPool);
    device.destroy(cameraDescriptorSetLayout);
    device.destroy(matricesDescriptorSetLayout);

    vma_allocator.destroyBuffer(cameraBuffer, cameraAllocation);
    vma_allocator.destroyBuffer(matricesBuffer, matricesAllocation);

    dynamicMeshes_uptr.reset();
    lights_uptr.reset();
    meshesOfNodes_uptr.reset();
    skinsOfMeshes_uptr.reset();
    materialsOfPrimitives_uptr.reset();
    texturesOfMaterials_uptr.reset();
    shadersSetsFamiliesCache_uptr.reset();
    pipelinesFactory_uptr.reset();
    primitivesOfMeshes_uptr.reset();
}

void Graphics::InitBuffers()
{
    // Create camera buffer
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof(glm::mat4) * 3 * 4;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              cameraAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        cameraBuffer = createBuffer_result.value.first;
        cameraAllocation = createBuffer_result.value.second;
    }

    // Create matrices buffer
    {
#ifdef ENABLE_ASYNC_COMPUTE
        std::vector<uint32_t> share_families_indices = {graphicsQueue.second, computeQueue.second};
#else
        std::vector<uint32_t> share_families_indices = {graphicsQueue.second};
#endif

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof(ModelMatrices) * maxInstances * 4;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        if (share_families_indices.size() > 1) {
            buffer_create_info.sharingMode = vk::SharingMode::eConcurrent;
            buffer_create_info.setQueueFamilyIndices(share_families_indices);
        } else {
            buffer_create_info.sharingMode = vk::SharingMode::eExclusive;
        }

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              matricesAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        matricesBuffer = createBuffer_result.value.first;
        matricesAllocation = createBuffer_result.value.second;
    }
}

void Graphics::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eUniformBuffer, 8);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 8);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 8,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create camera layout
        std::vector<vk::DescriptorSetLayoutBinding> buffer_bindings;

        vk::DescriptorSetLayoutBinding buffer_binding;
        buffer_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        buffer_binding.descriptorCount = 1;
        buffer_binding.stageFlags = vk::ShaderStageFlagBits::eVertex
                | vk::ShaderStageFlagBits::eFragment
                | vk::ShaderStageFlagBits::eCompute;

        buffer_binding.binding = 0;
        buffer_bindings.emplace_back(buffer_binding);
        buffer_binding.binding = 1;
        buffer_bindings.emplace_back(buffer_binding);

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, buffer_bindings);
        cameraDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Create matrices layout
        std::vector<vk::DescriptorSetLayoutBinding> buffer_bindings;

        vk::DescriptorSetLayoutBinding buffer_binding;
        buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
        buffer_binding.descriptorCount = 1;
        buffer_binding.stageFlags = vk::ShaderStageFlagBits::eVertex
                | vk::ShaderStageFlagBits::eFragment
                | vk::ShaderStageFlagBits::eCompute;

        buffer_binding.binding = 0;
        buffer_bindings.emplace_back(buffer_binding);
        buffer_binding.binding = 1;
        buffer_bindings.emplace_back(buffer_binding);

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, buffer_bindings);
        matricesDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Allocate descriptor sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        cameraDescriptorSets[0] = descriptor_sets[0];
        cameraDescriptorSets[1] = descriptor_sets[1];
        cameraDescriptorSets[2] = descriptor_sets[2];
        cameraDescriptorSets[3] = descriptor_sets[3];
        matricesDescriptorSets[0] = descriptor_sets[4];
        matricesDescriptorSets[1] = descriptor_sets[5];
        matricesDescriptorSets[2] = descriptor_sets[6];
        matricesDescriptorSets[3] = descriptor_sets[7];
    }

    {   // Writing descriptor set
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;

        for (size_t i = 0; i != 4; ++i) {
            {
                auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
                descriptor_buffer_info_uptr->buffer = cameraBuffer;
                descriptor_buffer_info_uptr->offset = i * 3 * sizeof(glm::mat4);
                descriptor_buffer_info_uptr->range = 3 * sizeof(glm::mat4);

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = cameraDescriptorSets[i];
                write_descriptor_set.dstBinding = 0;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eUniformBuffer;
                write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

                descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
            }
            {
                auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
                descriptor_buffer_info_uptr->buffer = cameraBuffer;
                descriptor_buffer_info_uptr->offset = ((i == 0)? 3 : (i - 1)) * 3 * sizeof(glm::mat4);
                descriptor_buffer_info_uptr->range = 3 * sizeof(glm::mat4);

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = cameraDescriptorSets[i];
                write_descriptor_set.dstBinding = 1;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eUniformBuffer;
                write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

                descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
            }
        }

        for (size_t i = 0; i != 4; ++i) {
            {
                auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
                descriptor_buffer_info_uptr->buffer = matricesBuffer;
                descriptor_buffer_info_uptr->offset = i * sizeof(ModelMatrices) * maxInstances;
                descriptor_buffer_info_uptr->range = sizeof(ModelMatrices) * maxInstances;

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = matricesDescriptorSets[i];
                write_descriptor_set.dstBinding = 0;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
                write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

                descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
            }
            {
                auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
                descriptor_buffer_info_uptr->buffer = matricesBuffer;
                descriptor_buffer_info_uptr->offset = ((i - 1) % 4) * sizeof(ModelMatrices) * maxInstances;
                descriptor_buffer_info_uptr->range = sizeof(ModelMatrices) * maxInstances;

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = matricesDescriptorSets[i];
                write_descriptor_set.dstBinding = 1;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
                write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

                descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
            }
        }

        device.updateDescriptorSets(writes_descriptor_set, {});
    }
}

void Graphics::InitPipelinesFactory()
{
    pipelinesFactory_uptr = std::make_unique<PipelinesFactory>(device);
}

void Graphics::InitShadersSetsFamiliesCache()
{
    shadersSetsFamiliesCache_uptr = std::make_unique<ShadersSetsFamiliesCache>(device, engine_ptr->GetVendorId(),  "shaders");

    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Histogram Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "histogramShader_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }

    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Dynamic Mesh Evaluation Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "dynamicMeshShader_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Offline Renderer - Visibility Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererOffline/visibilityPass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererOffline/visibilityPass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Offline Renderer - Shade-Pass Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererOffline/shadePass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererOffline/shadePass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Offline Renderer - Light Source Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererOffline/lightSourcePass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererOffline/lightSourcePass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Offline Renderer - ToneMap-Pass Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererOffline/toneMapPass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererOffline/toneMapPass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Realtime Renderer - Visibility Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererRealtime/visibilityPass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererRealtime/visibilityPass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Realtime Renderer - Path-Trace Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererRealtime/pathTracePass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererRealtime/pathTracePass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Realtime Renderer - Light Draw Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "rendererRealtime/lightDrawPass_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "rendererRealtime/lightDrawPass_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Realtime Renderer - Resolve Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "rendererRealtime/resolveShader_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Realtime Renderer - Morphological AA Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "rendererRealtime/morphologicalAA_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
}

void Graphics::InitMeshesTree()
{
    animationsDataOfNodes_uptr = std::make_unique<AnimationsDataOfNodes>();

    texturesOfMaterials_uptr = std::make_unique<TexturesOfMaterials>(device, vma_allocator, engine_ptr->GetQueuesList().graphicsQueues[0]);

    materialsOfPrimitives_uptr = std::make_unique<MaterialsOfPrimitives>(texturesOfMaterials_uptr.get(), device, vma_allocator);

    primitivesOfMeshes_uptr = std::make_unique<PrimitivesOfMeshes>(materialsOfPrimitives_uptr.get(), device, vma_allocator);

    skinsOfMeshes_uptr = std::make_unique<SkinsOfMeshes>(device, vma_allocator);

    meshesOfNodes_uptr = std::make_unique<MeshesOfNodes>(primitivesOfMeshes_uptr.get(), device, vma_allocator);

}

void Graphics::InitDynamicMeshes()
{
    dynamicMeshes_uptr = std::make_unique<DynamicMeshes>(this, device, vma_allocator, 127);
}

void Graphics::InitLights()
{
    lights_uptr = std::make_unique<Lights>(this, device, vma_allocator,
                                           1024, std::numeric_limits<uint16_t>::max());
}

void Graphics::InitGraphicsComponents()
{
    {
        animationActorComp_uptr = std::make_unique<AnimationActorComp>(engine_ptr->GetECSwrapperPtr(),
                                                                       animationsDataOfNodes_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(animationActorComp_uptr.get());
    }
    {
        uint32_t width = engine_ptr->GetSwapchainCreateInfo().imageExtent.width;
        uint32_t height = engine_ptr->GetSwapchainCreateInfo().imageExtent.height;

        cameraComp_uptr = std::make_unique<CameraComp>(engine_ptr->GetECSwrapperPtr(),
                                                       glm::radians(cfgFile["graphicsSettings"]["FOV"].as_float()),
                                                       float(width) / float(height),
                                                       cfgFile["graphicsSettings"]["nearPlaneDistance"].as_float(),
                                                       cfgFile["graphicsSettings"]["farPlaneDistance"].as_float());
        engine_ptr->GetECSwrapperPtr()->AddComponent(cameraComp_uptr.get());
    }

    {
        modelDrawComp_uptr = std::make_unique<ModelDrawComp>(engine_ptr->GetECSwrapperPtr());
        engine_ptr->GetECSwrapperPtr()->AddComponent(modelDrawComp_uptr.get());
    }

    {
        dynamicMeshComp_uptr = std::make_unique<DynamicMeshComp>(engine_ptr->GetECSwrapperPtr(),
                                                                 dynamicMeshes_uptr.get(),
                                                                 meshesOfNodes_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(dynamicMeshComp_uptr.get());
    }

    {
        lightComp_uptr = std::make_unique<LightComp>(engine_ptr->GetECSwrapperPtr(),
                                                     lights_uptr.get());
        engine_ptr->GetECSwrapperPtr()->AddComponent(lightComp_uptr.get());
    }
}

void Graphics::InitRenderer()
{
    if (cfgFile["graphicsSettings"]["renderer"].as_string() == "offline") {
        printf("Renderer: offline\n");
        renderer_uptr = std::make_unique<OfflineRenderer>(this, device, vma_allocator);
    }
    else if (cfgFile["graphicsSettings"]["renderer"].as_string() == "realtime-reBLUR") {
        printf("Renderer: realtime-reBLUR\n");
        renderer_uptr = std::make_unique<RealtimeRenderer>(this, device, vma_allocator, nrd::Method::REBLUR_DIFFUSE_SPECULAR);
    }
    else {
        printf("Renderer: realtime-reLAX\n");
        renderer_uptr = std::make_unique<RealtimeRenderer>(this, device, vma_allocator, nrd::Method::RELAX_DIFFUSE_SPECULAR);
    }
}


void Graphics::LoadModel(const tinygltf::Model& in_model, std::string model_images_folder)
{
    printf("--Adding model textures and materials\n");
    materialsOfPrimitives_uptr->AddMaterialsOfModel(in_model, model_images_folder);

    printf("--Adding model skins\n");
    skinsOfMeshes_uptr->AddSkinsOfModel(in_model);

    printf("--Adding model meshes\n");
    meshesOfNodes_uptr->AddMeshesOfModel(in_model);
}

void Graphics::EndModelsLoad()
{
    // Flashing device
    std::cout << "Flashing device\n";
#ifdef ENABLE_ASYNC_COMPUTE
    materialsOfPrimitives_uptr->FlashDevice(graphicsQueue);
    primitivesOfMeshes_uptr->FlashDevice({graphicsQueue, computeQueue});
    meshesOfNodes_uptr->FlashDevice({graphicsQueue, computeQueue});
    skinsOfMeshes_uptr->FlashDevice(computeQueue);
    dynamicMeshes_uptr->FlashDevice(computeQueue);
#else
    materialsOfPrimitives_uptr->FlashDevice(graphicsQueue);
    primitivesOfMeshes_uptr->FlashDevice({graphicsQueue});
    meshesOfNodes_uptr->FlashDevice({graphicsQueue});
    skinsOfMeshes_uptr->FlashDevice(graphicsQueue);
    dynamicMeshes_uptr->FlashDevice(graphicsQueue);
#endif

    std::cout << "Initializing renderer\n";
    InitRenderer();
}

void Graphics::DrawFrame()
{
    ViewportFrustum camera_viewport = cameraComp_uptr->GetBindedCameraEntity()->cameraViewportFrustum;

    std::vector<ModelMatrices> matrices;
    std::vector<LightInfo> light_infos;
    std::vector<DrawInfo> draw_infos;
    lightComp_uptr->AddLightInfos(camera_viewport.GetViewMatrix(), matrices, light_infos);
    modelDrawComp_uptr->AddDrawInfos(camera_viewport.GetViewMatrix(), matrices, draw_infos);

    renderer_uptr->DrawFrame(camera_viewport, std::move(matrices), std::move(light_infos), std::move(draw_infos));

    if (not renderer_uptr->IsFreezed()) {
        dynamicMeshes_uptr->CompleteRemovesSafe();
    }
}

void Graphics::ToggleCullingDebugging()
{
    cameraComp_uptr->ToggleCullingDebugging();
}

std::vector<vk::ImageView> Graphics::GetSwapchainImageViews() const
{
    return engine_ptr->GetSwapchainImageViews();
}

std::vector<vk::Image> Graphics::GetSwapchainImages() const
{
    return engine_ptr->GetSwapchainImages();
}

vk::SwapchainCreateInfoKHR Graphics::GetSwapchainCreateInfo() const
{
    return engine_ptr->GetSwapchainCreateInfo();
}

QueuesList Graphics::GetQueuesList() const
{
    return engine_ptr->GetQueuesList();
}

vk::SwapchainKHR Graphics::GetSwapchain() const
{
    return engine_ptr->GetSwapchain();
}

void Graphics::ToggleViewportFreeze()
{
    assert(renderer_uptr.get());
    renderer_uptr->ToggleViewportFreeze();
}

size_t Graphics::GetSubgroupSize() const
{
    vk::PhysicalDevice physical_device = engine_ptr->GetPhysicalDevice();

    vk::PhysicalDeviceProperties2 device_properties;
    vk::PhysicalDeviceVulkan11Properties device_properties_vulkan11;
    device_properties.pNext = &device_properties_vulkan11;

    physical_device.getProperties2(&device_properties);

    return device_properties_vulkan11.subgroupSize;
}

void Graphics::WriteCameraMarticesBuffers(ViewportFrustum viewport,
                                          const std::vector<ModelMatrices>& model_matrices,
                                          const std::vector<DrawInfo>& draw_infos,
                                          size_t buffer_index)
{
    buffer_index = buffer_index % 4;

    // Update camera matrix
    {
        glm::mat4x4 view_projection_matrices[3] = {viewport.GetViewMatrix(), glm::mat4(), viewport.GetPerspectiveMatrix()};
        view_projection_matrices[1] = glm::inverse(view_projection_matrices[0]);

        memcpy((std::byte*)(cameraAllocInfo.pMappedData) + buffer_index * 3 * sizeof(glm::mat4),
               view_projection_matrices,
               3 * sizeof(glm::mat4));
        vma_allocator.flushAllocation(cameraAllocation, buffer_index * 3 * sizeof(glm::mat4), 3 * sizeof(glm::mat4));
    }

    // Update model_matrices
    {
        memcpy((std::byte *) (matricesAllocInfo.pMappedData) + buffer_index * maxInstances * sizeof(ModelMatrices),
               model_matrices.data(),
               model_matrices.size() * sizeof(ModelMatrices));

        vma_allocator.flushAllocation(matricesAllocation, buffer_index * maxInstances * sizeof(ModelMatrices), model_matrices.size() * sizeof(ModelMatrices));
    }
}

float Graphics::GetDeltaTimeSeconds() const
{
    return engine_ptr->GetECSdeltaTime().count();
}




