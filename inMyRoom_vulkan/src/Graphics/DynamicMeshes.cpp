#include "Graphics/DynamicMeshes.h"
#include "Graphics/Graphics.h"
#include "Graphics/HelperUtils.h"
#include <iostream>

#include "common/structs/AABB.h"

DynamicMeshes::DynamicMeshes(Graphics* in_graphics_ptr,
                             vk::Device in_device,
                             vma::Allocator in_vma_allocator,
                             size_t in_max_dynamicMeshes)
    : graphics_ptr(in_graphics_ptr),
      device(in_device),
      vma_allocator(in_vma_allocator),
      max_dynamicMeshes(in_max_dynamicMeshes),
      waveSize(graphics_ptr->GetSubgroupSize()),
      accumulateLocalSize(1024)
{
}

DynamicMeshes::~DynamicMeshes()
{
    assert(hasBeenFlashed);

    device.destroy(verticesDescriptorSetLayout);
    device.destroy(AABBsAndScratchDescriptorSetLayout);
    device.destroy(descriptorPool);

    for (const auto& this_pair : indexToDynamicMeshInfo_umap) {
        vma_allocator.destroyBuffer(this_pair.second.buffer, this_pair.second.allocation);
        if(this_pair.second.hasDynamicShape) {
            device.destroy(this_pair.second.BLASesHandles[0]);
            device.destroy(this_pair.second.BLASesHandles[1]);
            vma_allocator.destroyBuffer(this_pair.second.BLASesBuffer, this_pair.second.BLASesAllocation);
            vma_allocator.destroyBuffer(this_pair.second.updateScratchBuffer, this_pair.second.updateScratchAllocation);

            vma_allocator.destroyBuffer(this_pair.second.AABBsBuffer, this_pair.second.AABBsAllocation);
        }
    }

    vma_allocator.destroyBuffer(AABBinitDataBuffer, AABBinitDataAllocation);
}


void DynamicMeshes::FlashDevice(std::pair<vk::Queue, uint32_t> queue)
{
    assert(not hasBeenFlashed);

    queue_family_index = queue.second;

    auto max_descriptor_per_set = uint32_t(max_dynamicMeshes + 1);
    // Description sets
    {   // Create descriptor pool
        vk::DescriptorPoolSize descriptor_pool_size = {vk::DescriptorType::eStorageBuffer, max_descriptor_per_set * (3 + 3)};
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 6,
                                                                 1 , &descriptor_pool_size);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }
    // verticesDescriptorSets
    {   // Create descriptor layouts (verticesDescriptorSets)
        vk::DescriptorSetLayoutBinding binding_layout;
        binding_layout.binding = 0;
        binding_layout.descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_layout.descriptorCount = max_descriptor_per_set;
        binding_layout.stageFlags = vk::ShaderStageFlagBits::eFragment | vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags;
        vk::DescriptorBindingFlags flags =  vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound;
        binding_flags.bindingCount = 1;
        binding_flags.pBindingFlags = &flags;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, 1, &binding_layout);
        descriptor_set_layout_create_info.pNext = &binding_flags;

        verticesDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Create descriptor sets (verticesDescriptorSets)
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        descriptor_set_layouts.emplace_back(verticesDescriptorSetLayout);
        descriptor_set_layouts.emplace_back(verticesDescriptorSetLayout);
        descriptor_set_layouts.emplace_back(verticesDescriptorSetLayout);

        uint32_t counts[3];
        counts[0] = max_descriptor_per_set - 1;
        counts[1] = max_descriptor_per_set - 1;
        counts[2] = max_descriptor_per_set - 1;

        vk::DescriptorSetVariableDescriptorCountAllocateInfo set_counts;
        set_counts.descriptorSetCount = 3;
        set_counts.pDescriptorCounts = counts;

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, descriptor_set_layouts);
        descriptor_set_allocate_info.pNext = &set_counts;

        auto descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        verticesDescriptorSets[0] = descriptor_sets[0];
        verticesDescriptorSets[1] = descriptor_sets[1];
        verticesDescriptorSets[2] = descriptor_sets[2];
    }
    // AABBsAndScratchDescriptorSets
    {   // Create descriptor layouts (AABBsAndScratchDescriptorSets)
        vk::DescriptorSetLayoutBinding binding_layout;
        binding_layout.binding = 0;
        binding_layout.descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_layout.descriptorCount = max_descriptor_per_set - 1;
        binding_layout.stageFlags = vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutBindingFlagsCreateInfo binding_flags;
        vk::DescriptorBindingFlags flags =  vk::DescriptorBindingFlagBits::eVariableDescriptorCount | vk::DescriptorBindingFlagBits::ePartiallyBound;
        binding_flags.bindingCount = 1;
        binding_flags.pBindingFlags = &flags;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, 1, &binding_layout);
        descriptor_set_layout_create_info.pNext = &binding_flags;

        AABBsAndScratchDescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Create descriptor sets (AABBsAndScratchDescriptorSets)
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        descriptor_set_layouts.emplace_back(AABBsAndScratchDescriptorSetLayout);
        descriptor_set_layouts.emplace_back(AABBsAndScratchDescriptorSetLayout);
        descriptor_set_layouts.emplace_back(AABBsAndScratchDescriptorSetLayout);

        uint32_t counts[3];
        counts[0] = max_descriptor_per_set - 1;
        counts[1] = max_descriptor_per_set - 1;
        counts[2] = max_descriptor_per_set - 1;

        vk::DescriptorSetVariableDescriptorCountAllocateInfo set_counts;
        set_counts.descriptorSetCount = 3;
        set_counts.pDescriptorCounts = counts;

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, descriptor_set_layouts);
        descriptor_set_allocate_info.pNext = &set_counts;

        auto descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        AABBsAndScratchDescriptorSets[0] = descriptor_sets[0];
        AABBsAndScratchDescriptorSets[1] = descriptor_sets[1];
        AABBsAndScratchDescriptorSets[2] = descriptor_sets[2];
    }

    // Compute pipelines
    {   // Pipelines layout
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        std::vector<vk::PushConstantRange> push_constant_range;
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMatricesDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetSkinsOfMeshesPtr()->GetDescriptorSetLayout());
        descriptor_sets_layouts.emplace_back(this->GetDescriptorLayout());
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eCompute, 0, uint32_t( sizeof(DynamicMeshComputePushConstants)) );
        {
            pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);
            pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

            generic_computeLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
        }
        {
            descriptor_sets_layouts.emplace_back(this->GetAABBsDescriptorLayout());
            pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);
            pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

            position_computeLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
        }
    }
    {   // Create pipelines
        std::vector<std::pair<std::string, std::string>> commonDefinitionStringPairs;
        commonDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
        commonDefinitionStringPairs.emplace_back("INVERSE_MATRICES_COUNT", std::to_string(graphics_ptr->GetSkinsOfMeshesPtr()->GetCountOfInverseBindMatrices()));
        commonDefinitionStringPairs.emplace_back("MAX_MORPH_WEIGHTS", std::to_string(maxMorphWeights));
        commonDefinitionStringPairs.emplace_back("WAVE_SIZE", std::to_string(waveSize));
        {   // Position compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("USE_SKIN", "");
            shaderDefinitionStringPairs.emplace_back("AABB_ACCUMULATE", "");
            shaderDefinitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(accumulateLocalSize));
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::PipelineShaderStageRequiredSubgroupSizeCreateInfoEXT require_stable_subgroup_size;
            require_stable_subgroup_size.requiredSubgroupSize = waveSize;

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.pNext = &require_stable_subgroup_size;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = position_computeLayout;

            positionCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
        {   // Normal compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("USE_SKIN", "");
            shaderDefinitionStringPairs.emplace_back("USE_NORMAL_MATRIX", "");
            shaderDefinitionStringPairs.emplace_back("ZERO_W", "");
            shaderDefinitionStringPairs.emplace_back("NORMALIZE", "");
            shaderDefinitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(waveSize));
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = generic_computeLayout;

            normalCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
        {   // Tangent compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("USE_SKIN", "");
            shaderDefinitionStringPairs.emplace_back("ZERO_W", "");
            shaderDefinitionStringPairs.emplace_back("NORMALIZE", "");
            shaderDefinitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(waveSize));
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = generic_computeLayout;

            tangentCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
        {   // texcoord compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("USE_VEC2", "");
            shaderDefinitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(waveSize));
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = generic_computeLayout;

            texcoordsCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
        {   // color compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(waveSize));
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = generic_computeLayout;

            colorCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
    }

    // Create AABB init data buffer
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = sizeof(AABB);
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eTransferDst
                | vk::BufferUsageFlagBits::eTransferSrc;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        AABBinitDataBuffer = createBuffer_result.value.first;
        AABBinitDataAllocation = createBuffer_result.value.second;
    }
    {
        StagingBuffer staging_buffer(device, vma_allocator, sizeof(AABB));

        AABBintCasted AABB_init = {};
        AABB_init.max_coords_intCasted = glm::ivec3(0x80000000, 0x80000000, 0x80000000);
        AABB_init.min_coords_intCasted = glm::ivec3(0x7FFFFFFF, 0x7FFFFFFF, 0x7FFFFFFF);
        memcpy(staging_buffer.GetDstPtr(), &AABB_init, sizeof(AABB));

        vk::CommandBuffer command_buffer = staging_buffer.BeginCommandRecord(queue);
        vk::Buffer copy_buffer = staging_buffer.GetBuffer();

        vk::BufferCopy copy_region;
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = sizeof(AABB);
        command_buffer.copyBuffer(copy_buffer, AABBinitDataBuffer, 1, &copy_region);

        staging_buffer.EndAndSubmitCommands();
    }

    hasBeenFlashed = true;
}

const DynamicMeshInfo& DynamicMeshes::GetDynamicMeshInfo(size_t index) const
{
    assert(indexToDynamicMeshInfo_umap.find(index) != indexToDynamicMeshInfo_umap.end());

    auto search = indexToDynamicMeshInfo_umap.find(index);
    return search->second;
}

size_t DynamicMeshes::AddDynamicMesh(size_t mesh_index)
{
    assert(hasBeenFlashed);

    if (indexToDynamicMeshInfo_umap.size() == max_dynamicMeshes) {
        std::cerr << "Exceeding dynamic meshes count!\n";
        std::terminate();
    }

    // Primitives info creation
    std::vector<DynamicMeshInfo::DynamicPrimitiveInfo> dynamicPrimitiveInfos;

    size_t buffer_offset = 0;
    std::vector<size_t> primitives_info_indices = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).primitivesIndex;
    for (size_t this_primitiveInfo_index : primitives_info_indices) {
        const PrimitiveInfo& this_primitiveInfo = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_primitiveInfo_index);
        bool isSkin = this_primitiveInfo.jointsCount;

        DynamicMeshInfo::DynamicPrimitiveInfo this_dynamicPrimitiveInfo;
        this_dynamicPrimitiveInfo.primitiveIndex = this_primitiveInfo_index;

        if (this_primitiveInfo.positionMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.positionByteOffset = buffer_offset;
            buffer_offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.normalMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.normalByteOffset = buffer_offset;
            buffer_offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.tangentMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.tangentByteOffset = buffer_offset;
            buffer_offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.texcoordsMorphTargets > 0) {
            this_dynamicPrimitiveInfo.texcoordsCount  = this_primitiveInfo.texcoordsCount;

            size_t size_in_byte = this_primitiveInfo.verticesCount * sizeof(float) * 2 * this_primitiveInfo.texcoordsCount;
            size_in_byte += (size_in_byte % 16 == 8) ? 8 : 0;

            this_dynamicPrimitiveInfo.texcoordsByteOffset = buffer_offset;
            buffer_offset += size_in_byte;
        }
        if (this_primitiveInfo.colorMorphTargets > 0) {
            this_dynamicPrimitiveInfo.colorByteOffset = buffer_offset;
            buffer_offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }

        dynamicPrimitiveInfos.emplace_back(this_dynamicPrimitiveInfo);
    }

    DynamicMeshInfo dynamicMeshInfo;
    dynamicMeshInfo.meshIndex = mesh_index;
    dynamicMeshInfo.dynamicPrimitives = std::move(dynamicPrimitiveInfos);
    // Create vertices buffer
    {
        size_t buffer_size = 2 * buffer_offset;
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = buffer_size;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eVertexBuffer
                | vk::BufferUsageFlagBits::eStorageBuffer
                | vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo allocation_create_info;
        allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        dynamicMeshInfo.buffer =  createBuffer_result.value.first;
        dynamicMeshInfo.allocation = createBuffer_result.value.second;
        dynamicMeshInfo.halfSize = buffer_size / 2;
    }

    // BLAS creation
    dynamicMeshInfo.hasDynamicShape = std::find_if(dynamicMeshInfo.dynamicPrimitives.begin(), dynamicMeshInfo.dynamicPrimitives.end(),
                                                   [](const auto& primitive) {return primitive.positionByteOffset != -1;}) != dynamicMeshInfo.dynamicPrimitives.end()
                                      && graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).meshBLAS.hasBLAS;
    if (dynamicMeshInfo.hasDynamicShape) {
        dynamicMeshInfo.BLASesHalfSize = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).meshBLAS.bufferSize;
        size_t scratch_buffer_size = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).meshBLAS.updateScratchBufferSize;

        // Create buffer for acceleration structure
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = dynamicMeshInfo.BLASesHalfSize * 2;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
            buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

            auto create_buffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
            assert(create_buffer_result.result == vk::Result::eSuccess);
            dynamicMeshInfo.BLASesBuffer = create_buffer_result.value.first;
            dynamicMeshInfo.BLASesAllocation = create_buffer_result.value.second;

        }

        // Create acceleration structures
        for (size_t i = 0; i != 2; ++i) {
            vk::AccelerationStructureCreateInfoKHR BLAS_create_info;
            BLAS_create_info.buffer = dynamicMeshInfo.BLASesBuffer;
            BLAS_create_info.size = dynamicMeshInfo.BLASesHalfSize;
            BLAS_create_info.offset = i * dynamicMeshInfo.BLASesHalfSize;
            BLAS_create_info.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
            auto BLAS_create_result = device.createAccelerationStructureKHR(BLAS_create_info);
            assert(BLAS_create_result.result == vk::Result::eSuccess);
            dynamicMeshInfo.BLASesHandles[i] = BLAS_create_result.value;
            dynamicMeshInfo.BLASesDeviceAddresses[i] = device.getAccelerationStructureAddressKHR({dynamicMeshInfo.BLASesHandles[i]});
        }

        // Create update scratch buffer
        {
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = scratch_buffer_size;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
            buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

            auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
            assert(createBuffer_result.result == vk::Result::eSuccess);
            dynamicMeshInfo.updateScratchBuffer = createBuffer_result.value.first;
            dynamicMeshInfo.updateScratchAllocation = createBuffer_result.value.second;
        }

        // AABBs buffer
        {
            size_t buffer_size = 2 * sizeof(AABB) * primitives_info_indices.size();
            vk::BufferCreateInfo buffer_create_info;
            buffer_create_info.size = buffer_size;
            buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer
                    | vk::BufferUsageFlagBits::eTransferSrc
                    | vk::BufferUsageFlagBits::eTransferDst;
            buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

            vma::AllocationCreateInfo allocation_create_info;
            allocation_create_info.usage = vma::MemoryUsage::eGpuToCpu;
            allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

            auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                                  allocation_create_info,
                                                                  dynamicMeshInfo.AABBsAllocInfo);
            assert(createBuffer_result.result == vk::Result::eSuccess);
            dynamicMeshInfo.AABBsBuffer = createBuffer_result.value.first;
            dynamicMeshInfo.AABBsAllocation = createBuffer_result.value.second;
            dynamicMeshInfo.AABBsBufferHalfsize = buffer_size / 2;
        }
    }

    size_t index = indexCounter++;
    indexToDynamicMeshInfo_umap.emplace(index, dynamicMeshInfo);

    return index;
}

void DynamicMeshes::RemoveDynamicMeshSafe(size_t index)
{
    assert(hasBeenFlashed);

    assert(indexToDynamicMeshInfo_umap.find(index) != indexToDynamicMeshInfo_umap.end());
    assert(indexToDynamicMeshInfo_umap.find(index)->second.shouldBeDeleted == false);
    indexToDynamicMeshInfo_umap.find(index)->second.shouldBeDeleted = true;

    indexToBeRemovedCountdown.emplace_back(index, removeCountdown);
}

void DynamicMeshes::SwapDescriptorSet(size_t swap_index)
{
    assert(hasBeenFlashed);

    swapIndex = swap_index;

    size_t device_buffer_index = swapIndex % 2;
    std::vector<vk::DescriptorBufferInfo> vertices_descriptor_buffer_infos;
    std::vector<vk::DescriptorBufferInfo> AABBsAndScratch_descriptor_buffer_infos;

    {   // Static primitives buffer
        vk::DescriptorBufferInfo primitives_buffer_info;
        primitives_buffer_info.buffer = graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer();
        primitives_buffer_info.offset = 0;
        primitives_buffer_info.range = VK_WHOLE_SIZE;
        vertices_descriptor_buffer_infos.emplace_back(primitives_buffer_info);
    }

    size_t index = 0;
    for (auto& this_pair : indexToDynamicMeshInfo_umap) {
        if (not this_pair.second.shouldBeDeleted) {
            {
                vk::Buffer buffer = this_pair.second.buffer;
                size_t halfSize = this_pair.second.halfSize;

                vk::DescriptorBufferInfo this_vertices_descriptor_buffer_info;
                this_vertices_descriptor_buffer_info.buffer = buffer;
                this_vertices_descriptor_buffer_info.offset = device_buffer_index * halfSize;
                this_vertices_descriptor_buffer_info.range = halfSize;

                vertices_descriptor_buffer_infos.emplace_back(this_vertices_descriptor_buffer_info);
            }

            if (this_pair.second.hasDynamicShape) {
                vk::Buffer AABBs_buffer = this_pair.second.AABBsBuffer;
                size_t AABBs_halfSize = this_pair.second.AABBsBufferHalfsize;
                vk::DescriptorBufferInfo this_AABBs_descriptor_buffer_info;
                this_AABBs_descriptor_buffer_info.buffer = AABBs_buffer;
                this_AABBs_descriptor_buffer_info.offset = device_buffer_index * AABBs_halfSize;
                this_AABBs_descriptor_buffer_info.range = AABBs_halfSize;

                AABBsAndScratch_descriptor_buffer_infos.emplace_back(this_AABBs_descriptor_buffer_info);
            } else {
                vk::DescriptorBufferInfo null_descriptor_buffer_info;
                null_descriptor_buffer_info.buffer = VK_NULL_HANDLE;
                null_descriptor_buffer_info.offset = 0;
                null_descriptor_buffer_info.range = VK_WHOLE_SIZE;

                AABBsAndScratch_descriptor_buffer_infos.emplace_back(null_descriptor_buffer_info);
            }

            this_pair.second.descriptorIndexOffset = index++;
        }
    }

    std::vector<vk::WriteDescriptorSet> write_descriptor_sets;
    std::vector<vk::CopyDescriptorSet> copy_descriptor_sets;
    if (vertices_descriptor_buffer_infos.size()){
        vk::WriteDescriptorSet write_descriptor_set;
        write_descriptor_set.dstSet = this->GetDescriptorSet();
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = uint32_t(vertices_descriptor_buffer_infos.size());
        write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
        write_descriptor_set.pBufferInfo = vertices_descriptor_buffer_infos.data();
        write_descriptor_sets.emplace_back(write_descriptor_set);
    }
    if (AABBsAndScratch_descriptor_buffer_infos.size()){
        vk::WriteDescriptorSet write_descriptor_set;
        write_descriptor_set.dstSet = this->GetAABBsDescriptorSet();
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = uint32_t(AABBsAndScratch_descriptor_buffer_infos.size());
        write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
        write_descriptor_set.pBufferInfo = AABBsAndScratch_descriptor_buffer_infos.data();
        write_descriptor_sets.emplace_back(write_descriptor_set);
    }

    device.updateDescriptorSets(write_descriptor_sets, copy_descriptor_sets);
}

void DynamicMeshes::CompleteRemovesSafe()
{
    assert(hasBeenFlashed);

    std::vector<std::pair<size_t, uint32_t>> new_bufferToBeRemovedCountdown;
    for(const auto& this_pair : indexToBeRemovedCountdown) {
        if (this_pair.second == 0) {
            assert(indexToDynamicMeshInfo_umap.find(this_pair.first) != indexToDynamicMeshInfo_umap.end());

            DynamicMeshInfo& this_dynamicMeshInfo = indexToDynamicMeshInfo_umap.find(this_pair.first)->second;
            assert(this_dynamicMeshInfo.shouldBeDeleted == true);

            vma_allocator.destroyBuffer(this_dynamicMeshInfo.buffer, this_dynamicMeshInfo.allocation);
            if (this_dynamicMeshInfo.hasDynamicShape) {
                device.destroy(this_dynamicMeshInfo.BLASesHandles[0]);
                device.destroy(this_dynamicMeshInfo.BLASesHandles[1]);
                vma_allocator.destroyBuffer(this_dynamicMeshInfo.BLASesBuffer, this_dynamicMeshInfo.BLASesAllocation);
                vma_allocator.destroyBuffer(this_dynamicMeshInfo.updateScratchBuffer, this_dynamicMeshInfo.updateScratchAllocation);

                vma_allocator.destroyBuffer(this_dynamicMeshInfo.AABBsBuffer, this_dynamicMeshInfo.AABBsAllocation);
            }

            indexToDynamicMeshInfo_umap.erase(this_pair.first);
        } else {
            new_bufferToBeRemovedCountdown.emplace_back(this_pair.first,
                                                        this_pair.second - 1);
        }
    }

    indexToBeRemovedCountdown = std::move(new_bufferToBeRemovedCountdown);
}

void DynamicMeshes::RecordTransformations(vk::CommandBuffer command_buffer,
                                          const std::vector<DrawInfo>& draw_infos) const
{
    assert(hasBeenFlashed);
    size_t device_buffer_index = swapIndex % 2;
    size_t host_buffer_index = swapIndex % 3;

    // Clear AABBs
    std::vector<vk::BufferMemoryBarrier> AABBs_clear_memory_barriers;
    for (const auto &draw_info: draw_infos) {
        const DynamicMeshInfo &dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);

        if (dynamic_mesh_info.hasDynamicShape) {
            std::vector<vk::BufferCopy> regions;
            for (size_t i = 0; i != dynamic_mesh_info.dynamicPrimitives.size(); ++i) {
                vk::BufferCopy this_region = {};
                this_region.srcOffset = 0;
                this_region.dstOffset = device_buffer_index * dynamic_mesh_info.AABBsBufferHalfsize + i * sizeof(AABB);
                this_region.size = sizeof(AABB);

                regions.emplace_back(this_region);
            }
            command_buffer.copyBuffer(AABBinitDataBuffer, dynamic_mesh_info.AABBsBuffer, regions);

            vk::BufferMemoryBarrier this_memory_barrier;
            this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
            this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
            this_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            this_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            this_memory_barrier.buffer = dynamic_mesh_info.AABBsBuffer;
            this_memory_barrier.offset = device_buffer_index * dynamic_mesh_info.AABBsBufferHalfsize;
            this_memory_barrier.size = dynamic_mesh_info.AABBsBufferHalfsize;

            AABBs_clear_memory_barriers.emplace_back(this_memory_barrier);
        }
    }

    if (AABBs_clear_memory_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                       vk::PipelineStageFlagBits::eComputeShader,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       AABBs_clear_memory_barriers,
                                       {});
    }

    // Calculate transformations
    std::vector<vk::DescriptorSet> generic_descriptor_sets;
    generic_descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(host_buffer_index));
    generic_descriptor_sets.emplace_back(graphics_ptr->GetSkinsOfMeshesPtr()->GetDescriptorSet());
    generic_descriptor_sets.emplace_back(this->GetDescriptorSet());

    for (const auto &draw_info: draw_infos) {
        const DynamicMeshInfo &dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);
        for (size_t i = 0; i != dynamic_mesh_info.dynamicPrimitives.size(); ++i) {
            const DynamicMeshInfo::DynamicPrimitiveInfo &this_dynamic_primitive = dynamic_mesh_info.dynamicPrimitives[i];
            const PrimitiveInfo &this_primitive = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive.primitiveIndex);

            std::vector<vk::DescriptorSet> position_descriptor_sets = generic_descriptor_sets;
            position_descriptor_sets.emplace_back(this->GetAABBsDescriptorSet());
            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, position_computeLayout, 0, position_descriptor_sets, {});
            if (this_dynamic_primitive.positionByteOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, positionCompPipeline);

                assert(this_primitive.positionByteOffset % (sizeof(float) * 4) == 0);
                assert(this_primitive.jointsByteOffset % (sizeof(uint16_t) * 4) == 0);
                assert(this_primitive.weightsByteOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.positionByteOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.positionByteOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = uint32_t(this_primitive.jointsByteOffset / (sizeof(uint16_t) * 4));
                push_constants.weightsOffset = uint32_t(this_primitive.weightsByteOffset / (sizeof(float) * 4));
                push_constants.jointsGroupsCount = uint32_t(this_primitive.jointsCount);
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndexOffset);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.positionByteOffset / (sizeof(float) * 4));
                push_constants.AABBresultOffset = uint32_t(i);
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(position_computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch(uint32_t(this_primitive.verticesCount + accumulateLocalSize - 1) / accumulateLocalSize, 1, 1);
            }

            command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, generic_computeLayout, 0, generic_descriptor_sets, {});
            if (this_dynamic_primitive.normalByteOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, normalCompPipeline);

                assert(this_primitive.normalByteOffset % (sizeof(float) * 4) == 0);
                assert(this_primitive.jointsByteOffset % (sizeof(uint16_t) * 4) == 0);
                assert(this_primitive.weightsByteOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.normalByteOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.normalByteOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = uint32_t(this_primitive.jointsByteOffset / (sizeof(uint16_t) * 4));
                push_constants.weightsOffset = uint32_t(this_primitive.weightsByteOffset / (sizeof(float) * 4));
                push_constants.jointsGroupsCount = uint32_t(this_primitive.jointsCount);
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndexOffset);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.normalByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(generic_computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch(uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.tangentByteOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, tangentCompPipeline);

                assert(this_primitive.tangentByteOffset % (sizeof(float) * 4) == 0);
                assert(this_primitive.jointsByteOffset % (sizeof(uint16_t) * 4) == 0);
                assert(this_primitive.weightsByteOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.tangentByteOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.tangentByteOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = uint32_t(this_primitive.jointsByteOffset / (sizeof(uint16_t) * 4));
                push_constants.weightsOffset = uint32_t(this_primitive.weightsByteOffset / (sizeof(float) * 4));
                push_constants.jointsGroupsCount = uint32_t(this_primitive.jointsCount);
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndexOffset);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.tangentByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(generic_computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch(uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.colorByteOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, colorCompPipeline);

                assert(this_primitive.colorByteOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.colorByteOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.colorByteOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = 0;
                push_constants.weightsOffset = 0;
                push_constants.jointsGroupsCount = 0;
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndexOffset);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.colorByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(generic_computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch(uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.texcoordsByteOffset != -1) {
                for (size_t j = 0; j != this_dynamic_primitive.texcoordsCount; j++) {
                    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, texcoordsCompPipeline);

                    assert(this_primitive.texcoordsByteOffset % (sizeof(float) * 4) == 0);
                    assert(this_dynamic_primitive.texcoordsByteOffset % (sizeof(float) * 4) == 0);

                    DynamicMeshComputePushConstants push_constants;
                    push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                    push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                    push_constants.verticesOffset = uint32_t(this_primitive.texcoordsByteOffset / (sizeof(float) * 2) + j);
                    push_constants.jointsOffset = 0;
                    push_constants.weightsOffset = 0;
                    push_constants.jointsGroupsCount = 0;
                    push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                    push_constants.size_x = uint32_t(this_primitive.verticesCount);
                    push_constants.step_multiplier = uint32_t(this_dynamic_primitive.texcoordsCount);
                    push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndexOffset);
                    push_constants.resultOffset = uint32_t(this_dynamic_primitive.texcoordsByteOffset / (sizeof(float) * 4) + j);
                    std::copy(draw_info.weights.begin(),
                              draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                              push_constants.morph_weights.begin());
                    command_buffer.pushConstants(generic_computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                    command_buffer.dispatch(uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
                }
            }
        }
    }

    // Make AABBs host visible
    std::vector<vk::BufferMemoryBarrier> AABBs_hostvisible_memory_barriers;
    for (const auto &draw_info: draw_infos) {
        const DynamicMeshInfo &dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);

        if (dynamic_mesh_info.hasDynamicShape) {
            vk::BufferMemoryBarrier this_memory_barrier;
            this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
            this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eHostRead;
            this_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            this_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            this_memory_barrier.buffer = dynamic_mesh_info.AABBsBuffer;
            this_memory_barrier.offset = device_buffer_index * dynamic_mesh_info.AABBsBufferHalfsize;
            this_memory_barrier.size = dynamic_mesh_info.AABBsBufferHalfsize;

            AABBs_hostvisible_memory_barriers.emplace_back(this_memory_barrier);
        }
    }

    if (AABBs_hostvisible_memory_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                       vk::PipelineStageFlagBits::eHost,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       AABBs_hostvisible_memory_barriers,
                                       {});
    }
}

void DynamicMeshes::RecordBLASupdate(vk::CommandBuffer command_buffer,
                                     const std::vector<DrawInfo>& draw_infos) const
{
    assert(hasBeenFlashed);
    size_t device_buffer_index = swapIndex % 2;
    size_t host_buffer_index = swapIndex % 3;

    // Update BLASes
    std::vector<vk::BufferMemoryBarrier> BLAS_memory_barries;
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> infos;
    std::vector<std::unique_ptr<std::vector<vk::AccelerationStructureGeometryKHR>>> geometries_of_infos;
    std::vector<std::unique_ptr<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>> ranges_of_infos;
    uint64_t static_buffer_address = device.getBufferAddress(graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer());
    for (const auto& draw_info : draw_infos) {
        const MeshInfo& mesh_info = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(draw_info.meshIndex);
        const DynamicMeshInfo& dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);

        if (dynamic_mesh_info.hasDynamicShape) {
            uint64_t dynamic_buffer_address = device.getBufferAddress(dynamic_mesh_info.buffer);

            std::vector<vk::AccelerationStructureGeometryKHR> geometries;
            std::vector<vk::AccelerationStructureBuildRangeInfoKHR> geometries_ranges;
            for (const DynamicMeshInfo::DynamicPrimitiveInfo &this_dynamic_primitive_info: dynamic_mesh_info.dynamicPrimitives) {
                const PrimitiveInfo& this_primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);

                const MaterialAbout& material_about = graphics_ptr->GetMaterialsOfPrimitives()->GetMaterialAbout(this_primitive_info.material);

                vk::AccelerationStructureGeometryKHR geometry;
                geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
                geometry.flags = (not material_about.masked && not material_about.transparent) ? vk::GeometryFlagBitsKHR::eOpaque : vk::GeometryFlagsKHR(0);
                geometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
                geometry.geometry.triangles.vertexData = this_dynamic_primitive_info.positionByteOffset != -1
                        ? dynamic_buffer_address + (swapIndex % 2) * dynamic_mesh_info.halfSize + this_dynamic_primitive_info.positionByteOffset
                        : static_buffer_address + this_primitive_info.positionByteOffset;
                geometry.geometry.triangles.vertexStride = sizeof(glm::vec4);
                geometry.geometry.triangles.maxVertex = this_primitive_info.verticesCount;
                geometry.geometry.triangles.indexType = vk::IndexType::eUint32;
                geometry.geometry.triangles.indexData = static_buffer_address + this_primitive_info.indicesByteOffset;
                geometry.geometry.triangles.transformData = nullptr;

                vk::AccelerationStructureBuildRangeInfoKHR range;
                range.primitiveCount = this_primitive_info.indicesCount / 3;
                range.primitiveOffset = 0;
                range.firstVertex = 0;
                range.transformOffset = 0;

                geometries.emplace_back(geometry);
                geometries_ranges.emplace_back(range);
            }
            geometries_of_infos.emplace_back(std::make_unique<std::vector<vk::AccelerationStructureGeometryKHR>>(std::move(geometries)));
            ranges_of_infos.emplace_back(std::make_unique<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>(std::move(geometries_ranges)));

            vk::AccelerationStructureBuildGeometryInfoKHR geometry_info;
            geometry_info.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
            geometry_info.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate;
            geometry_info.mode = vk::BuildAccelerationStructureModeKHR::eUpdate;
            geometry_info.scratchData.deviceAddress = device.getBufferAddress(dynamic_mesh_info.updateScratchBuffer);
            geometry_info.srcAccelerationStructure = mesh_info.meshBLAS.handle;
            geometry_info.dstAccelerationStructure = dynamic_mesh_info.BLASesHandles[(swapIndex % 2)];
            geometry_info.setGeometries(*geometries_of_infos.back());

            infos.emplace_back(geometry_info);

            vk::BufferMemoryBarrier this_memory_barrier;
            this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
            this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
            this_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            this_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            this_memory_barrier.buffer = dynamic_mesh_info.BLASesBuffer;
            this_memory_barrier.offset = device_buffer_index * dynamic_mesh_info.BLASesHalfSize;
            this_memory_barrier.size = dynamic_mesh_info.BLASesHalfSize;

            BLAS_memory_barries.emplace_back(this_memory_barrier);
        }
    }

    if (infos.size()) {
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> ranges_of_infos_ptrs;
        std::transform(ranges_of_infos.begin(), ranges_of_infos.end(), std::back_inserter(ranges_of_infos_ptrs),
                       [](const auto& uptr) { return uptr->data(); });

        command_buffer.buildAccelerationStructuresKHR(infos, ranges_of_infos_ptrs);

        // Ready for TLAS update
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       BLAS_memory_barries,
                                       {});
    }
}

void DynamicMeshes::ObtainTransformRanges(vk::CommandBuffer command_buffer,
                                          const std::vector<DrawInfo>& draw_infos,
                                          uint32_t source_family_index) const
{
    assert(hasBeenFlashed);
    size_t buffer_index = swapIndex % 2;

    if (queue_family_index == source_family_index)
        return;

    // Obtain ownerships
    std::vector<vk::BufferMemoryBarrier> ownership_obtain_memory_barriers;
    for (auto this_barrier: GetGenericTransformRangesBarriers(draw_infos, buffer_index)) {
        this_barrier.dstAccessMask = vk::AccessFlagBits::eShaderWrite;
        this_barrier.srcQueueFamilyIndex = source_family_index;
        ownership_obtain_memory_barriers.emplace_back(this_barrier);
    }

    if (ownership_obtain_memory_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                       vk::PipelineStageFlagBits::eComputeShader,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       ownership_obtain_memory_barriers,
                                       {});
    }
}

void DynamicMeshes::ObtainBLASranges(vk::CommandBuffer command_buffer,
                                     const std::vector<DrawInfo> &draw_infos,
                                     uint32_t source_family_index) const
{
    assert(hasBeenFlashed);
    size_t buffer_index = swapIndex % 2;

    if (queue_family_index == source_family_index)
        return;

    // Obtain ownerships
    std::vector<vk::BufferMemoryBarrier> ownership_obtain_memory_barriers;
    for (auto this_barrier: GetGenericBLASrangesBarriers(draw_infos, buffer_index)) {
        this_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
        this_barrier.srcQueueFamilyIndex = source_family_index;
        ownership_obtain_memory_barriers.emplace_back(this_barrier);
    }

    if (ownership_obtain_memory_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                       vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       ownership_obtain_memory_barriers,
                                       {});
    }
}

void DynamicMeshes::TransferTransformAndBLASranges(vk::CommandBuffer command_buffer,
                                                   const std::vector<DrawInfo> &draw_infos,
                                                   uint32_t dst_family_index) const
{
    assert(hasBeenFlashed);
    size_t buffer_index = swapIndex % 2;

    if (queue_family_index == dst_family_index)
        return;

    // Transfer ownerships
    std::vector<vk::BufferMemoryBarrier> ownership_give_memory_barriers;
    for (auto this_barrier: GetGenericTransformRangesBarriers(draw_infos, buffer_index)) {
        this_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureReadKHR;
        this_barrier.dstQueueFamilyIndex = dst_family_index;
        ownership_give_memory_barriers.emplace_back(this_barrier);
    }
    for (auto this_barrier: GetGenericBLASrangesBarriers(draw_infos, buffer_index)) {
        this_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
        this_barrier.dstQueueFamilyIndex = dst_family_index;
        ownership_give_memory_barriers.emplace_back(this_barrier);
    }

    if (ownership_give_memory_barriers.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::PipelineStageFlagBits::eBottomOfPipe,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       ownership_give_memory_barriers,
                                       {});
    }
}

std::vector<vk::BufferMemoryBarrier> DynamicMeshes::GetGenericTransformRangesBarriers(const std::vector<DrawInfo> &draw_infos,
                                                                                      uint32_t buffer_index) const
{
    std::vector<vk::BufferMemoryBarrier> barriers;
    for (const auto &draw_info: draw_infos) {
        const DynamicMeshInfo &dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);
        {
            vk::BufferMemoryBarrier this_memory_barrier;
            this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
            this_memory_barrier.srcQueueFamilyIndex = queue_family_index;
            this_memory_barrier.dstQueueFamilyIndex = queue_family_index;
            this_memory_barrier.buffer = dynamic_mesh_info.buffer;
            this_memory_barrier.offset = buffer_index * dynamic_mesh_info.halfSize;
            this_memory_barrier.size = dynamic_mesh_info.halfSize;

            barriers.emplace_back(this_memory_barrier);
        }
    }

    return barriers;
}

std::vector<vk::BufferMemoryBarrier> DynamicMeshes::GetGenericBLASrangesBarriers(const std::vector<DrawInfo> &draw_infos, uint32_t buffer_index) const
{
    std::vector<vk::BufferMemoryBarrier> barriers;
    for (const auto &draw_info: draw_infos) {
        const DynamicMeshInfo &dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);
        if (dynamic_mesh_info.hasDynamicShape) {
            vk::BufferMemoryBarrier this_memory_barrier;
            this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
            this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
            this_memory_barrier.srcQueueFamilyIndex = queue_family_index;
            this_memory_barrier.dstQueueFamilyIndex = queue_family_index;
            this_memory_barrier.buffer = dynamic_mesh_info.BLASesBuffer;
            this_memory_barrier.offset = buffer_index * dynamic_mesh_info.BLASesHalfSize;
            this_memory_barrier.size = dynamic_mesh_info.BLASesHalfSize;

            barriers.emplace_back(this_memory_barrier);
        }
    }

    return barriers;
}
