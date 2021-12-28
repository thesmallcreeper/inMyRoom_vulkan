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

    for (const auto& this_pair : bufferToVMAallocation_umap) {
        vma_allocator.destroyBuffer(this_pair.first, this_pair.second);
    }
}


void DynamicMeshes::FlashDevice()
{
    assert(not hasBeenFlashed);

    uint32_t max_descriptor_per_set = uint32_t(max_dynamicMeshes + 1);

    // Description sets
    {   // Create descriptor pool
        vk::DescriptorPoolSize descriptor_pool_size = {vk::DescriptorType::eStorageBuffer, max_descriptor_per_set * 2};
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2,
                                                                 1 , &descriptor_pool_size);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }
    {   // Create descriptor layouts
        vk::DescriptorSetLayoutBinding binding_layout;
        binding_layout.binding = 0;
        binding_layout.descriptorType = vk::DescriptorType::eStorageBuffer;
        binding_layout.descriptorCount = max_descriptor_per_set;
        binding_layout.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute;

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

        uint32_t counts[2];
        counts[0] = max_descriptor_per_set - 1;
        counts[1] = max_descriptor_per_set - 1;

        vk::DescriptorSetVariableDescriptorCountAllocateInfo set_counts;
        set_counts.descriptorSetCount = 2;
        set_counts.pDescriptorCounts = counts;

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, descriptor_set_layouts);
        descriptor_set_allocate_info.pNext = &set_counts;

        auto descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        descriptorSets[0] = descriptor_sets[0];
        descriptorSets[1] = descriptor_sets[1];
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

const std::vector<DynamicPrimitiveInfo> &DynamicMeshes::GetDynamicPrimitivesInfo(size_t index) const
{
    assert(indexToDynamicPrimitivesInfos_umap.find(index) != indexToDynamicPrimitivesInfos_umap.end());

    auto search = indexToDynamicPrimitivesInfos_umap.find(index);
    return search->second;
}

size_t DynamicMeshes::AddDynamicMesh(const std::vector<size_t>& primitives_info_indices)
{
    assert(hasBeenFlashed);

    if (indexToDynamicPrimitivesInfos_umap.size() == max_dynamicMeshes) {
        std::cerr << "Exceeding dynamic meshes count!\n";
        std::terminate();
    }

    std::vector<DynamicPrimitiveInfo> dynamicPrimitiveInfos;

    size_t offset = 0;
    for (size_t this_primitiveInfo_index : primitives_info_indices) {
        const PrimitiveInfo& this_primitiveInfo = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_primitiveInfo_index);
        bool isSkin = this_primitiveInfo.jointsCount;

        DynamicPrimitiveInfo this_dynamicPrimitiveInfo;
        this_dynamicPrimitiveInfo.primitiveIndex = this_primitiveInfo_index;

        if (this_primitiveInfo.positionMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.positionOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.normalMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.normalOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.tangentMorphTargets > 0 || isSkin) {
            this_dynamicPrimitiveInfo.tangentOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }
        if (this_primitiveInfo.texcoordsMorphTargets > 0) {
            this_dynamicPrimitiveInfo.texcoordsCount  = this_primitiveInfo.texcoordsCount;

            size_t size_in_byte = this_primitiveInfo.verticesCount * sizeof(float) * 2 * this_primitiveInfo.texcoordsCount;
            size_in_byte += (size_in_byte % 16 == 8) ? 8 : 0;

            this_dynamicPrimitiveInfo.texcoordsOffset = offset;
            offset += size_in_byte;
        }
        if (this_primitiveInfo.colorMorphTargets > 0) {
            this_dynamicPrimitiveInfo.colorOffset = offset;
            offset += this_primitiveInfo.verticesCount * sizeof(float) * 4;
        }

        dynamicPrimitiveInfos.emplace_back(this_dynamicPrimitiveInfo);
    }

    size_t buffer_size = 2 * offset;

    vk::BufferCreateInfo buffer_create_info;
    buffer_create_info.size = buffer_size;
    buffer_create_info.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eStorageBuffer;
    buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo allocation_create_info;
    allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

    auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
    assert(createBuffer_result.result == vk::Result::eSuccess);
    vk::Buffer buffer = createBuffer_result.value.first;
    vma::Allocation allocation = createBuffer_result.value.second;

    for (auto& this_dynamicPrimitiveInfo : dynamicPrimitiveInfos) {
        this_dynamicPrimitiveInfo.halfSize = offset;
        this_dynamicPrimitiveInfo.buffer = buffer;
    }

    size_t index = indexCounter++;
    indexToDynamicPrimitivesInfos_umap.emplace(index, dynamicPrimitiveInfos);
    bufferToVMAallocation_umap.emplace(buffer, allocation);

    return index;
}

void DynamicMeshes::RemoveDynamicMeshSafe(size_t index)
{
    assert(hasBeenFlashed);

    assert(indexToDynamicPrimitivesInfos_umap.find(index) != indexToDynamicPrimitivesInfos_umap.end());
    indicesToBeRemovedInNextTwoRemoves.emplace_back(index);
}

void DynamicMeshes::SwapDescriptorSet()
{
    assert(hasBeenFlashed);

    ++swapsCounter;

    size_t descriptor_set_index = swapsCounter % 2;
    std::vector<vk::DescriptorBufferInfo> descriptor_buffer_infos;

    {   // Default primitives buffer
        vk::DescriptorBufferInfo primitives_buffer_info;
        primitives_buffer_info.buffer = graphics_ptr->GetPrimitivesOfMeshes()->GetVerticesBuffer();
        primitives_buffer_info.offset = 0;
        primitives_buffer_info.range = VK_WHOLE_SIZE;
        descriptor_buffer_infos.emplace_back(primitives_buffer_info);
    }

    for (auto& this_pair : indexToDynamicPrimitivesInfos_umap) {
        vk::Buffer buffer = this_pair.second[0].buffer;
        size_t halfSize = this_pair.second[0].halfSize;

        vk::DescriptorBufferInfo this_descriptor_buffer_info;
        this_descriptor_buffer_info.buffer = buffer;
        this_descriptor_buffer_info.offset = descriptor_set_index * halfSize;
        this_descriptor_buffer_info.range = halfSize;

        for (auto& this_dynamic_primitive : this_pair.second) {
            this_dynamic_primitive.descriptorIndex = descriptor_buffer_infos.size();
        }

        descriptor_buffer_infos.emplace_back(this_descriptor_buffer_info);
    }

    vk::WriteDescriptorSet write_descriptor_set;
    write_descriptor_set.dstSet = descriptorSets[descriptor_set_index];
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

    for(size_t index : indicesToBeRemovedInNextRemoves) {
        assert(indexToDynamicPrimitivesInfos_umap.find(index) != indexToDynamicPrimitivesInfos_umap.end());
        vk::Buffer buffer = indexToDynamicPrimitivesInfos_umap.find(index)->second[0].buffer;

        assert(bufferToVMAallocation_umap.find(buffer) != bufferToVMAallocation_umap.end());
        vma::Allocation allocation = bufferToVMAallocation_umap.find(buffer)->second;

        vma_allocator.destroyBuffer(buffer, allocation);
        indexToDynamicPrimitivesInfos_umap.erase(index);
        bufferToVMAallocation_umap.erase(buffer);
    }

    indicesToBeRemovedInNextRemoves.clear();
    std::swap(indicesToBeRemovedInNextRemoves, indicesToBeRemovedInNextTwoRemoves);
}

void DynamicMeshes::RecordTransformations(vk::CommandBuffer command_buffer,
                                           const std::vector<DrawInfo>& draw_infos)
{
    assert(hasBeenFlashed);

    std::vector<vk::DescriptorSet> descriptor_sets;
    descriptor_sets.emplace_back(graphics_ptr->GetMatricesDescriptionSet());
    descriptor_sets.emplace_back(graphics_ptr->GetSkinsOfMeshesPtr()->GetDescriptorSet());
    descriptor_sets.emplace_back(this->GetDescriptorSet());

    command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, computeLayout, 0, descriptor_sets, {});

    std::vector<vk::BufferMemoryBarrier> buffer_memory_barries;
    for (const auto& draw_info : draw_infos) {
        for (const DynamicPrimitiveInfo& this_dynamic_primitive : GetDynamicPrimitivesInfo(draw_info.dynamicMeshIndex)) {
            const PrimitiveInfo& this_primitive = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive.primitiveIndex);

            if (this_dynamic_primitive.positionOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, positionCompPipeline);

                assert(this_primitive.positionOffset % (sizeof(float) * 4) == 0);
                assert(this_primitive.jointsOffset % (sizeof(uint16_t) * 4) == 0);
                assert(this_primitive.weightsOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.positionOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.positionOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = uint32_t(this_primitive.jointsOffset / (sizeof(uint16_t) * 4));
                push_constants.weightsOffset = uint32_t(this_primitive.weightsOffset / (sizeof(float) * 4));
                push_constants.jointsGroupsCount = uint32_t(this_primitive.jointsCount);
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(this_dynamic_primitive.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.positionOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.normalOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, normalCompPipeline);

                assert(this_primitive.normalOffset % (sizeof(float) * 4) == 0);
                assert(this_primitive.jointsOffset % (sizeof(uint16_t) * 4) == 0);
                assert(this_primitive.weightsOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.normalOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.normalOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = uint32_t(this_primitive.jointsOffset / (sizeof(uint16_t) * 4));
                push_constants.weightsOffset = uint32_t(this_primitive.weightsOffset / (sizeof(float) * 4));
                push_constants.jointsGroupsCount = uint32_t(this_primitive.jointsCount);
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(this_dynamic_primitive.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.normalOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.tangentOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, tangentCompPipeline);

                assert(this_primitive.tangentOffset % (sizeof(float) * 4) == 0);
                assert(this_primitive.jointsOffset % (sizeof(uint16_t) * 4) == 0);
                assert(this_primitive.weightsOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.tangentOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.tangentOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = uint32_t(this_primitive.jointsOffset / (sizeof(uint16_t) * 4));
                push_constants.weightsOffset = uint32_t(this_primitive.weightsOffset / (sizeof(float) * 4));
                push_constants.jointsGroupsCount = uint32_t(this_primitive.jointsCount);
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(this_dynamic_primitive.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.tangentOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.colorOffset != -1) {
                command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, colorCompPipeline);

                assert(this_primitive.colorOffset % (sizeof(float) * 4) == 0);
                assert(this_dynamic_primitive.colorOffset % (sizeof(float) * 4) == 0);

                DynamicMeshComputePushConstants push_constants;
                push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                push_constants.verticesOffset = uint32_t(this_primitive.colorOffset / (sizeof(float) * 4));
                push_constants.jointsOffset = 0;
                push_constants.weightsOffset = 0;
                push_constants.jointsGroupsCount = 0;
                push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                push_constants.size_x = uint32_t(this_primitive.verticesCount);
                push_constants.step_multiplier = 1;
                push_constants.resultDescriptorIndex = uint32_t(this_dynamic_primitive.descriptorIndex);
                push_constants.resultOffset = uint32_t(this_dynamic_primitive.colorOffset / (sizeof(float) * 4));
                std::copy(draw_info.weights.begin(),
                          draw_info.weights.begin() + std::min(draw_info.weights.size(), push_constants.morph_weights.size()),
                          push_constants.morph_weights.begin());
                command_buffer.pushConstants(computeLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(DynamicMeshComputePushConstants), &push_constants);

                command_buffer.dispatch( uint32_t(this_primitive.verticesCount + waveSize - 1) / waveSize, 1, 1);
            }

            if (this_dynamic_primitive.texcoordsOffset != -1) {
                for(size_t i = 0; i != this_dynamic_primitive.texcoordsCount; i++) {
                    command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, texcoordsCompPipeline);

                    assert(this_primitive.texcoordsOffset % (sizeof(float) * 4) == 0);
                    assert(this_dynamic_primitive.texcoordsOffset % (sizeof(float) * 4) == 0);

                    DynamicMeshComputePushConstants push_constants;
                    push_constants.matrixOffset = uint32_t(draw_info.matricesOffset);
                    push_constants.inverseMatricesOffset = uint32_t(draw_info.inverseMatricesOffset);
                    push_constants.verticesOffset = uint32_t(this_primitive.texcoordsOffset / (sizeof(float) * 2) + i);
                    push_constants.jointsOffset = 0;
                    push_constants.weightsOffset = 0;
                    push_constants.jointsGroupsCount = 0;
                    push_constants.morphTargets = std::min(uint32_t(draw_info.weights.size()), maxMorphWeights);
                    push_constants.size_x = uint32_t(this_primitive.verticesCount);
                    push_constants.step_multiplier = uint32_t(this_dynamic_primitive.texcoordsCount);
                    push_constants.resultDescriptorIndex = uint32_t(this_dynamic_primitive.descriptorIndex);
                    push_constants.resultOffset = uint32_t(this_dynamic_primitive.texcoordsOffset / (sizeof(float) * 4) + i);
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
        this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eVertexAttributeRead;
        this_memory_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        this_memory_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        this_memory_barrier.buffer = GetDynamicPrimitivesInfo(draw_info.dynamicMeshIndex)[0].buffer;
        this_memory_barrier.offset = (swapsCounter % 2) * GetDynamicPrimitivesInfo(draw_info.dynamicMeshIndex)[0].halfSize;
        this_memory_barrier.size = GetDynamicPrimitivesInfo(draw_info.dynamicMeshIndex)[0].halfSize;

        buffer_memory_barries.emplace_back(this_memory_barrier);
    }

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                   vk::PipelineStageFlagBits::eVertexInput,
                                   vk::DependencyFlagBits::eByRegion,
                                   {},
                                   buffer_memory_barries,
                                   {});
}


