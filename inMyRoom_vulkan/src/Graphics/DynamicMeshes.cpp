#include "Graphics/DynamicMeshes.h"
#include "Graphics/Graphics.h"
#include <iostream>

DynamicMeshes::DynamicMeshes(Graphics* in_graphics_ptr,
                             vk::Device in_device,
                             vma::Allocator in_vma_allocator,
                             size_t in_max_dynamicMeshes)
    :graphics_ptr(in_graphics_ptr),
     device(in_device),
     vma_allocator(in_vma_allocator),
     max_dynamicMeshes(in_max_dynamicMeshes)
{
}

DynamicMeshes::~DynamicMeshes()
{
    assert(hasBeenFlashed);

    device.destroy(descriptorSetLayout);
    device.destroy(descriptorPool);

    for (const auto& this_pair : indexToDynamicMeshInfo_umap) {
        vma_allocator.destroyBuffer(this_pair.second.buffer, this_pair.second.allocation);
        if(this_pair.second.hasDynamicBLAS) {
            device.destroy(this_pair.second.BLASesHandles[0]);
            device.destroy(this_pair.second.BLASesHandles[1]);
            vma_allocator.destroyBuffer(this_pair.second.BLASesBuffer, this_pair.second.BLASesAllocation);
            vma_allocator.destroyBuffer(this_pair.second.updateScratchBuffer, this_pair.second.updateScratchAllocation);
        }
    }
}


void DynamicMeshes::FlashDevice()
{
    assert(not hasBeenFlashed);

    uint32_t max_descriptor_per_set = uint32_t(max_dynamicMeshes + 1);

    // Description sets
    {   // Create descriptor pool
        vk::DescriptorPoolSize descriptor_pool_size = {vk::DescriptorType::eStorageBuffer, max_descriptor_per_set * 3};
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 3,
                                                                 1 , &descriptor_pool_size);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }
    {   // Create descriptor layouts
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

        descriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }
    {   // Create descriptor sets
        std::vector<vk::DescriptorSetLayout> descriptor_set_layouts;
        descriptor_set_layouts.emplace_back(descriptorSetLayout);
        descriptor_set_layouts.emplace_back(descriptorSetLayout);
        descriptor_set_layouts.emplace_back(descriptorSetLayout);

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
        descriptorSets[0] = descriptor_sets[0];
        descriptorSets[1] = descriptor_sets[1];
        descriptorSets[2] = descriptor_sets[2];
    }

    // Compute pipelines
    {   // Pipelines layout
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetMatricesDescriptionSetLayout());
        descriptor_sets_layouts.emplace_back(graphics_ptr->GetSkinsOfMeshesPtr()->GetDescriptorSetLayout());
        descriptor_sets_layouts.emplace_back(this->GetDescriptorLayout());
        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);

        std::vector<vk::PushConstantRange> push_constant_range;
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eCompute, 0, uint32_t( sizeof(DynamicMeshComputePushConstants)) );
        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        computeLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
    }
    {   // Create pipelines
        std::vector<std::pair<std::string, std::string>> commonDefinitionStringPairs;
        commonDefinitionStringPairs.emplace_back("MATRICES_COUNT", std::to_string(graphics_ptr->GetMaxInstancesCount()));
        commonDefinitionStringPairs.emplace_back("INVERSE_MATRICES_COUNT", std::to_string(graphics_ptr->GetSkinsOfMeshesPtr()->GetCountOfInverseBindMatrices()));
        commonDefinitionStringPairs.emplace_back("MAX_MORPH_WEIGHTS", std::to_string(maxMorphWeights));
        commonDefinitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(waveSize));
        {   // Position compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("USE_SKIN", "");
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = computeLayout;

            positionCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
        // TODO!
        normalCompPipeline = positionCompPipeline;
        tangentCompPipeline = positionCompPipeline;
        {   // texcoord compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            shaderDefinitionStringPairs.emplace_back("USE_VEC2", "");
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = computeLayout;

            texcoordsCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
        {   // color compute pipeline
            std::vector<std::pair<std::string, std::string>> shaderDefinitionStringPairs = commonDefinitionStringPairs;
            ShadersSpecs shaders_specs = {"Dynamic Mesh Evaluation Shader", shaderDefinitionStringPairs};
            ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

            vk::ComputePipelineCreateInfo compute_pipeline_create_info;
            compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
            compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
            compute_pipeline_create_info.stage.pName = "main";
            compute_pipeline_create_info.layout = computeLayout;

            colorCompPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
        }
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

    size_t offset = 0;
    std::vector<size_t> primitives_info_indices = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(mesh_index).primitivesIndex;
    for (size_t this_primitiveInfo_index : primitives_info_indices) {
        const PrimitiveInfo& this_primitiveInfo = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_primitiveInfo_index);
        bool isSkin = this_primitiveInfo.jointsCount;

        DynamicMeshInfo::DynamicPrimitiveInfo this_dynamicPrimitiveInfo;
        this_dynamicPrimitiveInfo.primitiveIndex = this_primitiveInfo_index;

        if (this_primitiveInfo.positionMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.positionByteOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.normalMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.normalByteOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.tangentMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.tangentByteOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.texcoordsMorphTargets > 0) {
            this_dynamicPrimitiveInfo.texcoordsCount  = this_primitiveInfo.texcoordsCount;

            size_t size_in_byte = this_primitiveInfo.verticesCount * sizeof(float) * 2 * this_primitiveInfo.texcoordsCount;
            size_in_byte += (size_in_byte % 16 == 8) ? 8 : 0;

            this_dynamicPrimitiveInfo.texcoordsByteOffset = offset;
            offset += size_in_byte;
        }
        if (this_primitiveInfo.colorMorphTargets > 0) {
            this_dynamicPrimitiveInfo.colorByteOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }

        dynamicPrimitiveInfos.emplace_back(this_dynamicPrimitiveInfo);
    }

    DynamicMeshInfo dynamicMeshInfo;
    dynamicMeshInfo.meshIndex = mesh_index;
    dynamicMeshInfo.dynamicPrimitives = std::move(dynamicPrimitiveInfos);
    // Create vertices buffer
    {
        size_t buffer_size = 2 * offset;
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = buffer_size;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
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
    dynamicMeshInfo.hasDynamicBLAS = std::find_if(dynamicMeshInfo.dynamicPrimitives.begin(), dynamicMeshInfo.dynamicPrimitives.end(),
                                                  [](const auto& primitive) {return primitive.positionByteOffset != -1;}) != dynamicMeshInfo.dynamicPrimitives.end();
    if (dynamicMeshInfo.hasDynamicBLAS) {
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
    }

    size_t index = indexCounter++;
    indexToDynamicMeshInfo_umap.emplace(index, dynamicMeshInfo);

    return index;
}

void DynamicMeshes::RemoveDynamicMeshSafe(size_t index)
{
    assert(hasBeenFlashed);

    assert(indexToDynamicMeshInfo_umap.find(index) != indexToDynamicMeshInfo_umap.end());
    dynamicMeshToBeRemovedCountdown.emplace_back(indexToDynamicMeshInfo_umap.find(index)->second, removeCountdown);

    indexToDynamicMeshInfo_umap.erase(index);
}

void DynamicMeshes::SwapDescriptorSet(size_t swap_index)
{
    assert(hasBeenFlashed);

    swapIndex = swap_index;

    size_t buffer_index = swapIndex % 2;
    std::vector<vk::DescriptorBufferInfo> descriptor_buffer_infos;

    {   // Static primitives buffer
        vk::DescriptorBufferInfo primitives_buffer_info;
        primitives_buffer_info.buffer = graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer();
        primitives_buffer_info.offset = 0;
        primitives_buffer_info.range = VK_WHOLE_SIZE;
        descriptor_buffer_infos.emplace_back(primitives_buffer_info);
    }

    for (auto& this_pair : indexToDynamicMeshInfo_umap) {
        vk::Buffer buffer = this_pair.second.buffer;
        size_t halfSize = this_pair.second.halfSize;

        vk::DescriptorBufferInfo this_descriptor_buffer_info;
        this_descriptor_buffer_info.buffer = buffer;
        this_descriptor_buffer_info.offset = buffer_index * halfSize;
        this_descriptor_buffer_info.range = halfSize;

        this_pair.second.descriptorIndex = descriptor_buffer_infos.size();

        descriptor_buffer_infos.emplace_back(this_descriptor_buffer_info);
    }

    vk::WriteDescriptorSet write_descriptor_set;
    write_descriptor_set.dstSet = this->GetDescriptorSet();
    write_descriptor_set.dstBinding = 0;
    write_descriptor_set.dstArrayElement = 0;
    write_descriptor_set.descriptorCount = uint32_t(descriptor_buffer_infos.size());
    write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
    write_descriptor_set.pBufferInfo = descriptor_buffer_infos.data();

    device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);
}

void DynamicMeshes::CompleteRemovesSafe()
{
    assert(hasBeenFlashed);

    std::vector<std::pair<DynamicMeshInfo, uint32_t>> new_bufferToBeRemovedCountdown;
    for(const auto& this_pair : dynamicMeshToBeRemovedCountdown) {
        if (this_pair.second == 0) {
            vma_allocator.destroyBuffer(this_pair.first.buffer, this_pair.first.allocation);
            if (this_pair.first.hasDynamicBLAS) {
                device.destroy(this_pair.first.BLASesHandles[0]);
                device.destroy(this_pair.first.BLASesHandles[1]);
                vma_allocator.destroyBuffer(this_pair.first.BLASesBuffer, this_pair.first.BLASesAllocation);
                vma_allocator.destroyBuffer(this_pair.first.updateScratchBuffer, this_pair.first.updateScratchAllocation);
            }
        } else {
            new_bufferToBeRemovedCountdown.emplace_back(this_pair.first,
                                                        this_pair.second - 1);
        }
    }

    dynamicMeshToBeRemovedCountdown = std::move(new_bufferToBeRemovedCountdown);
}

void DynamicMeshes::RecordTransformations(vk::CommandBuffer command_buffer,
                                           const std::vector<DrawInfo>& draw_infos)
{
    assert(hasBeenFlashed);

    // Calculate transformations
    std::vector<vk::DescriptorSet> descriptor_sets;
    descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet(swapIndex));
    descriptor_sets.emplace_back(graphics_ptr->GetSkinsOfMeshesPtr()->GetDescriptorSet());
    descriptor_sets.emplace_back(this->GetDescriptorSet());

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computeLayout, 0, descriptor_sets, {});

    std::vector<vk::BufferMemoryBarrier> buffer_memory_barries;
    for (const auto& draw_info : draw_infos) {
        const DynamicMeshInfo& dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);
        for (const DynamicMeshInfo::DynamicPrimitiveInfo& this_dynamic_primitive : dynamic_mesh_info.dynamicPrimitives) {
            const PrimitiveInfo& this_primitive = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive.primitiveIndex);

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
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.positionByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

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
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.normalByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
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
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.tangentByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
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
                push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.colorByteOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.texcoordsByteOffset != -1) {
                for(size_t i = 0; i != this_dynamic_primitive.texcoordsCount; i++) {
                    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, texcoordsCompPipeline);

                    assert(this_primitive.texcoordsByteOffset % (sizeof(float) * 4) == 0);
                    assert(this_dynamic_primitive.texcoordsByteOffset % (sizeof(float) * 4) == 0);

                    DynamicMeshComputePushConstants push_constants;
                    push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                    push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                    push_constants.verticesOffset = uint32_t(this_primitive.texcoordsByteOffset / (sizeof(float) * 2) + i);
                    push_constants.jointsOffset = 0;
                    push_constants.weightsOffset = 0;
                    push_constants.jointsGroupsCount = 0;
                    push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                    push_constants.size_x = uint32_t(this_primitive.verticesCount);
                    push_constants.step_multiplier = uint32_t(this_dynamic_primitive.texcoordsCount);
                    push_constants.resultDescriptorIndex = uint32_t(dynamic_mesh_info.descriptorIndex);
                    push_constants.resultOffset = uint32_t(this_dynamic_primitive.texcoordsByteOffset / (sizeof(float) * 4) + i);
                    std::copy(draw_info.weights.begin(),
                              draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                              push_constants.morph_weights.begin());
                    command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                    command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
                }
            }
        }

        vk::BufferMemoryBarrier this_memory_barrier;
        this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead | vk::AccessFlagBits::eAccelerationStructureWriteKHR;
        this_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        this_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        this_memory_barrier.buffer = dynamic_mesh_info.buffer;
        this_memory_barrier.offset = (swapIndex % 2) * dynamic_mesh_info.halfSize;
        this_memory_barrier.size = dynamic_mesh_info.halfSize;

        buffer_memory_barries.emplace_back(this_memory_barrier);
    }

    if (buffer_memory_barries.size()) {
        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                       vk::PipelineStageFlagBits::eVertexInput | vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       buffer_memory_barries,
                                       {});
    }

    // Update BLASes
    std::vector<vk::BufferMemoryBarrier> BLAS_memory_barries;
    std::vector<vk::AccelerationStructureBuildGeometryInfoKHR> infos;
    std::vector<std::unique_ptr<std::vector<vk::AccelerationStructureGeometryKHR>>> geometries_of_infos;
    std::vector<std::unique_ptr<std::vector<vk::AccelerationStructureBuildRangeInfoKHR>>> ranges_of_infos;
    uint64_t static_buffer_address = device.getBufferAddress(graphics_ptr->GetPrimitivesOfMeshes()->GetBuffer());
    for (const auto& draw_info : draw_infos) {
        const MeshInfo& mesh_info = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(draw_info.meshIndex);
        const DynamicMeshInfo& dynamic_mesh_info = GetDynamicMeshInfo(draw_info.dynamicMeshIndex);

        if (dynamic_mesh_info.hasDynamicBLAS) {
            uint64_t dynamic_buffer_address = device.getBufferAddress(dynamic_mesh_info.buffer);

            std::vector<vk::AccelerationStructureGeometryKHR> geometries;
            std::vector<vk::AccelerationStructureBuildRangeInfoKHR> geometries_ranges;
            for (const DynamicMeshInfo::DynamicPrimitiveInfo &this_dynamic_primitive_info: dynamic_mesh_info.dynamicPrimitives) {
                const PrimitiveInfo &this_primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);

                vk::AccelerationStructureGeometryKHR geometry;
                geometry.geometryType = vk::GeometryTypeKHR::eTriangles;
                geometry.geometry.triangles.vertexFormat = vk::Format::eR32G32B32Sfloat;
                geometry.geometry.triangles.vertexData = this_dynamic_primitive_info.positionByteOffset
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
            this_memory_barrier.offset = (swapIndex % 2) * dynamic_mesh_info.BLASesHalfSize;
            this_memory_barrier.size = dynamic_mesh_info.BLASesHalfSize;

            BLAS_memory_barries.emplace_back(this_memory_barrier);
        }
    }

    if (infos.size()) {
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR*> ranges_of_infos_ptrs;
        std::transform(ranges_of_infos.begin(), ranges_of_infos.end(), std::back_inserter(ranges_of_infos_ptrs),
                       [](const auto& uptr) { return uptr->data(); });

        command_buffer.buildAccelerationStructuresKHR(infos, ranges_of_infos_ptrs);

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                       vk::PipelineStageFlagBits::eRayTracingShaderKHR | vk::PipelineStageFlagBits::eFragmentShader,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       BLAS_memory_barries,
                                       {});
    }

}


