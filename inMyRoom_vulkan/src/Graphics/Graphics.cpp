#include "Graphics/Graphics.h"

#include "Geometry/FrustumCulling.h"
#include "Engine.h"
#include "Graphics/Renderers/Renderer.h"

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
    std::cout << "Initializing graphics oriented components\n";
    InitGraphicsComponents();
}

Graphics::~Graphics()
{
    device.waitIdle();

    device.destroy(descriptorPool);
    device.destroy(cameraDescriptorSetLayout);
    device.destroy(matricesDescriptorSetLayout);

    vma_allocator.destroyBuffer(cameraBuffer, cameraAllocation);
    vma_allocator.destroyBuffer(matricesBuffer, matricesAllocation);

    dynamicMeshes_uptr.reset();
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
        buffer_create_info.size = sizeof(glm::mat4) * 4;
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
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof(glm::mat4) * maxInstances * 2;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

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
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eUniformBuffer, 2);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 2);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 4,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create camera layout
        vk::DescriptorSetLayoutBinding buffer_binding;
        buffer_binding.binding = 0;
        buffer_binding.descriptorType = vk::DescriptorType::eUniformBuffer;
        buffer_binding.descriptorCount = 1;
        buffer_binding.stageFlags = vk::ShaderStageFlagBits::eVertex;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, 1, &buffer_binding);
        cameraDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Create matrices layout
        vk::DescriptorSetLayoutBinding buffer_binding;
        buffer_binding.binding = 0;
        buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
        buffer_binding.descriptorCount = 1;
        buffer_binding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, 1, &buffer_binding);
        matricesDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Allocate descriptor sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(cameraDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);
        layouts.emplace_back(matricesDescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        cameraDescriptorSets[0] = descriptor_sets[0];
        cameraDescriptorSets[1] = descriptor_sets[1];
        matricesDescriptorSets[0] = descriptor_sets[2];
        matricesDescriptorSets[1] = descriptor_sets[3];
    }

    {   // Writing descriptor set
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;

        {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = cameraBuffer;
            descriptor_buffer_info_uptr->offset = 0;
            descriptor_buffer_info_uptr->range  = 2 * sizeof(glm::mat4);

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = cameraDescriptorSets[0];
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
            descriptor_buffer_info_uptr->offset = 2 * sizeof(glm::mat4);
            descriptor_buffer_info_uptr->range  = 2 * sizeof(glm::mat4);

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = cameraDescriptorSets[1];
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
            descriptor_buffer_info_uptr->buffer = matricesBuffer;
            descriptor_buffer_info_uptr->offset = 0;
            descriptor_buffer_info_uptr->range  = sizeof(glm::mat4) * maxInstances;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = matricesDescriptorSets[0];
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
            descriptor_buffer_info_uptr->offset = sizeof(glm::mat4) * maxInstances;
            descriptor_buffer_info_uptr->range  = sizeof(glm::mat4) * maxInstances;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = matricesDescriptorSets[1];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
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
    shadersSetsFamiliesCache_uptr = std::make_unique<ShadersSetsFamiliesCache>(device, "shaders");

    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Texture-Pass Shaders";
        this_shaderSetInitInfo.fragmentShaderSourceFilename = "generalShader_glsl.frag";
        this_shaderSetInitInfo.vertexShaderSourceFilename = "generalShader_glsl.vert";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
    {
        ShadersSetsFamilyInitInfo this_shaderSetInitInfo;
        this_shaderSetInitInfo.shadersSetFamilyName = "Dynamic Mesh Evaluation Shader";
        this_shaderSetInitInfo.computeShaderSourceFilename = "dynamicMeshShader_glsl.comp";
        shadersSetsFamiliesCache_uptr->AddShadersSetsFamily(this_shaderSetInitInfo);
    }
}

void Graphics::InitMeshesTree()
{
    animationsDataOfNodes_uptr = std::make_unique<AnimationsDataOfNodes>();

    texturesOfMaterials_uptr = std::make_unique<TexturesOfMaterials>(device, vma_allocator, engine_ptr->GetQueuesList().graphicsQueues[0]);

    materialsOfPrimitives_uptr = std::make_unique<MaterialsOfPrimitives>(texturesOfMaterials_uptr.get(), device, vma_allocator);                                                               //needs flash

    primitivesOfMeshes_uptr = std::make_unique<PrimitivesOfMeshes>(materialsOfPrimitives_uptr.get(), device, vma_allocator);

    skinsOfMeshes_uptr = std::make_unique<SkinsOfMeshes>(device, vma_allocator);

    meshesOfNodes_uptr = std::make_unique<MeshesOfNodes>(primitivesOfMeshes_uptr.get());

}

void Graphics::InitDynamicMeshes()
{
    dynamicMeshes_uptr = std::make_unique<DynamicMeshes>(this, device, vma_allocator, 127);
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
}

void Graphics::InitRenderer()
{
    renderer_uptr = std::make_unique<Renderer>(this, device, vma_allocator);
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
    materialsOfPrimitives_uptr->FlashDevice(graphicsQueue);
    primitivesOfMeshes_uptr->FlashDevice(graphicsQueue);
    skinsOfMeshes_uptr->FlashDevice(graphicsQueue);
    dynamicMeshes_uptr->FlashDevice();

    std::cout << "Initializing renderer\n";
    InitRenderer();
}

void Graphics::DrawFrame()
{
    std::vector<glm::mat4> matrices;
    std::vector<DrawInfo> draw_infos;
    modelDrawComp_uptr->AddDrawInfos(matrices, draw_infos);

    ViewportFrustum camera_viewport = cameraComp_uptr->GetBindedCameraEntity()->cullingViewportFrustum;

    renderer_uptr->DrawFrame(camera_viewport, matrices, draw_infos);

    dynamicMeshes_uptr->CompleteRemovesSafe();
}

void Graphics::ToggleCullingDebugging()
{
    cameraComp_uptr->ToggleCullingDebugging();
}

std::vector<vk::ImageView> Graphics::GetSwapchainImageViews() const
{
    return engine_ptr->GetSwapchainImageViews();
}

vk::SwapchainCreateInfoKHR Graphics::GetSwapchainCreateInfo() const
{
    return engine_ptr->GetSwapchainCreateInfo();
}

QueuesList Graphics::GetQueuesList() const
{
    return engine_ptr->GetQueuesList();
}

vk::SwapchainKHR Graphics::GetSwapchain() const{
    return engine_ptr->GetSwapchain();
}

void Graphics::HostWriteBuffers(ViewportFrustum viewport,
                                const std::vector<glm::mat4>& matrices,
                                const std::vector<DrawInfo>& draw_infos,
                                size_t buffer_index)
{
    // Update camera matrix
    {
        glm::mat4x4 view_projection_matrices[2] = {viewport.GetViewMatrix(), viewport.GetPerspectiveMatrix()};

        memcpy((std::byte*)(cameraAllocInfo.pMappedData) + buffer_index * 2 * sizeof(glm::mat4),
               view_projection_matrices,
               2 * sizeof(glm::mat4));
        vma_allocator.flushAllocation(cameraAllocation, buffer_index * sizeof(glm::mat4), sizeof(glm::mat4));
    }

    // Update matrices
    {
        memcpy((std::byte *) (matricesAllocInfo.pMappedData) + buffer_index * maxInstances * sizeof(glm::mat4),
               matrices.data(),
               matrices.size() * sizeof(glm::mat4));

        vma_allocator.flushAllocation(matricesAllocation, buffer_index * maxInstances * sizeof(glm::mat4), matrices.size() * sizeof(glm::mat4));
    }
}



