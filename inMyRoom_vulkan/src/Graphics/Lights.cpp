#include "Graphics/Lights.h"

#include <cassert>
#include "common/structs/LightParameters.h"

Lights::Lights(struct Graphics *in_graphics_ptr,
               vk::Device in_device, vma::Allocator in_vma_allocator,
               size_t in_max_lights, size_t in_max_lightCombinationsSize)
    :graphics_ptr(in_graphics_ptr),
     device(in_device),
     vma_allocator(in_vma_allocator),
     max_lights(in_max_lights),
     max_lightCombinationsSize(in_max_lightCombinationsSize)
{
    assert(max_lights <= std::numeric_limits<uint16_t>::max());

    InitBuffers();
    InitDescriptors();
}

Lights::~Lights()
{
    device.destroy(descriptorPool);
    device.destroy(descriptorSetLayout);

    vma_allocator.destroyBuffer(lightsBuffer, lightsAllocation);
    vma_allocator.destroyBuffer(lightsCombinationsBuffer, lightsCombinationsAllocation);
}

size_t Lights::AddLightEntity()
{
    size_t light_index = indexCounter++;

    assert(lights_indices_uset.find(light_index) == lights_indices_uset.end());
    lights_indices_uset.emplace(light_index);

    return light_index;
}

void Lights::RemoveLightEntitySafe(size_t index)
{
    assert(lights_indices_uset.find(index) != lights_indices_uset.end());
    lights_indices_uset.erase(index);
}

void Lights::InitBuffers()
{
    // Create light buffer
    {
        lightsRangeSize = sizeof(LightParameters) * max_lights;
        lightsRangeSize += (lightsRangeSize % 16 == 0) ? 0 : 16 - lightsRangeSize % 16;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = lightsRangeSize * 3;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                                            buffer_allocation_create_info,
                                                                            lightsAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        lightsBuffer = createBuffer_result.value.first;
        lightsAllocation = createBuffer_result.value.second;
    }

    // Create light combinations buffer
    {
        lightsCombinationsRangeSize = sizeof(uint16_t) * max_lightCombinationsSize;
        lightsCombinationsRangeSize += (lightsCombinationsRangeSize % 16 == 0) ? 0 : 16 - lightsCombinationsRangeSize % 16;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = lightsCombinationsRangeSize * 3;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              lightsCombinationsAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        lightsCombinationsBuffer = createBuffer_result.value.first;
        lightsCombinationsAllocation = createBuffer_result.value.second;
    }

}

void Lights::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 6);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 3,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create descriptor layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {
            vk::DescriptorSetLayoutBinding buffer_binding;
            buffer_binding.binding = 0;
            buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
            buffer_binding.descriptorCount = 1;
            buffer_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(buffer_binding);
        }
        {
            vk::DescriptorSetLayoutBinding buffer_binding;
            buffer_binding.binding = 1;
            buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
            buffer_binding.descriptorCount = 1;
            buffer_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(buffer_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        descriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Allocate sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(descriptorSetLayout);
        layouts.emplace_back(descriptorSetLayout);
        layouts.emplace_back(descriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        descriptorSets[0] = descriptor_sets[0];
        descriptorSets[1] = descriptor_sets[1];
        descriptorSets[2] = descriptor_sets[2];
    }

    {   // Writing descriptor set
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;

        for (size_t i = 0; i != 3; ++i) {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = lightsBuffer;
            descriptor_buffer_info_uptr->offset = i * lightsRangeSize;
            descriptor_buffer_info_uptr->range  = lightsRangeSize;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = descriptorSets[i];
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            descriptor_buffer_infos_uptrs.emplace_back(std::move(descriptor_buffer_info_uptr));
            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        for (size_t i = 0; i != 3; ++i) {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = lightsCombinationsBuffer;
            descriptor_buffer_info_uptr->offset = i * lightsCombinationsRangeSize;
            descriptor_buffer_info_uptr->range  = lightsCombinationsRangeSize;

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = descriptorSets[i];
            write_descriptor_set.dstBinding = 1;
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

void Lights::PrepareNewFrame(size_t frame_index)
{
    frameIndex = frame_index;

    lightIndexToLightInfo_umap.clear();

    lightParameters.clear();
    lightsSpheres.clear();

    combinationToLightsIndicesRange.clear();
    lightsCombinationsIndices.clear();

    uniformLuminance = glm::vec3(0.f);
}

void Lights::AddLights(std::vector<LightInfo>& light_infos,
                       const std::vector<ModelMatrices> &model_matrices)
{
    for (auto& this_light_info : light_infos) {
        assert(lights_indices_uset.find(this_light_info.lightIndex) != lights_indices_uset.end());
        assert(lightIndexToLightInfo_umap.find(this_light_info.lightIndex) == lightIndexToLightInfo_umap.end());

        if (this_light_info.lightType != LightType::Uniform) {
            this_light_info.lightOffset = lightParameters.size();
        }
        lightIndexToLightInfo_umap.emplace(this_light_info.lightIndex, this_light_info);

        if (this_light_info.lightType != LightType::Uniform) {
            LightParameters this_lightParameters = {};
            this_lightParameters.lightType = uint8_t(this_light_info.lightType);
            this_lightParameters.luminance = this_light_info.luminance;
            this_lightParameters.radius = this_light_info.radius;
            this_lightParameters.length = this_light_info.length;
            this_lightParameters.range = this_light_info.range;
            this_lightParameters.matricesOffset = this_light_info.matricesOffset;

            lightParameters.emplace_back(this_lightParameters);

            glm::mat4 pos_matrix = model_matrices[this_light_info.matricesOffset].positionMatrix;
            auto sphere_pos = glm::vec3(pos_matrix[3]);
            float sphere_radius = this_light_info.range;
            Sphere this_rangeSphere = {sphere_pos, sphere_radius};

            lightsSpheres.emplace_back(this_rangeSphere);
        } else {
            uniformLuminance += this_light_info.luminance;
        }
    }
}

LightsIndicesRange Lights::CreateLightsConesRange()
{
    std::vector<uint16_t> cone_lights_indices;
    for (size_t i = 0; i != lightParameters.size(); ++i) {
        const auto& this_lightParameters = lightParameters[i];
        if (this_lightParameters.lightType == uint8_t(LightType::Cone)) {
            cone_lights_indices.emplace_back(uint16_t(i));
        }
    }

    LightsIndicesRange return_range = {};
    return_range.offset = uint32_t(lightsCombinationsIndices.size());
    return_range.size = uint32_t(cone_lights_indices.size());

    std::copy(cone_lights_indices.begin(), cone_lights_indices.end(),
              std::back_inserter(lightsCombinationsIndices));

    return return_range;
}

LightsIndicesRange Lights::CreateCollidedLightsRange(const Paralgram& paralgram)
{
    std::vector<uint16_t> collided_lights = CollideParalgramWithLocalLights(paralgram);

    LightsIndicesRange return_range = {};
    if (collided_lights.size() == 0)
        return return_range;

    auto search = combinationToLightsIndicesRange.find(collided_lights);
    if (search != combinationToLightsIndicesRange.end()) {
        return_range = search->second;
    } else {
        return_range.offset = uint32_t(lightsCombinationsIndices.size());
        return_range.size = uint32_t(collided_lights.size());

        std::copy(collided_lights.begin(), collided_lights.end(),
                  std::back_inserter(lightsCombinationsIndices));

        assert(lightsCombinationsIndices.size() <= max_lightCombinationsSize);

        combinationToLightsIndicesRange.emplace(std::move(collided_lights), return_range);
    }

    return return_range;
}

void Lights::WriteLightsBuffers() const
{
    size_t hostVisible_buffer_index = frameIndex % 3;

    { // Lights parameters
        memcpy((std::byte*)(lightsAllocInfo.pMappedData) + hostVisible_buffer_index * lightsRangeSize,
               lightParameters.data(),
               sizeof(LightParameters) * lightParameters.size());
        vma_allocator.flushAllocation(lightsAllocation, hostVisible_buffer_index * lightsRangeSize, lightsRangeSize);
    }

    { // Lights combinations
        memcpy((std::byte*)(lightsCombinationsAllocInfo.pMappedData) + hostVisible_buffer_index * lightsCombinationsRangeSize,
               lightsCombinationsIndices.data(),
               sizeof(uint16_t) * lightsCombinationsIndices.size());
        vma_allocator.flushAllocation(lightsCombinationsAllocation, hostVisible_buffer_index * lightsCombinationsRangeSize, lightsCombinationsRangeSize);
    }
}

std::vector<uint16_t> Lights::CollideParalgramWithLocalLights(const Paralgram &paralgram)
{
    assert(lightsSpheres.size() < std::numeric_limits<uint16_t>::max());

    std::vector<uint16_t> return_vector;
    for (size_t i = 0; i != lightsSpheres.size(); ++i) {
        const auto& this_lightParameters = lightParameters[i];
        const auto& this_sphere = lightsSpheres[i];
        if (this_lightParameters.lightType != uint8_t(LightType::Cone)
         && this_sphere.IntersectParalgram(paralgram)) {
            return_vector.emplace_back(uint16_t(i));
        }
    }

    return return_vector;
}

const LightInfo &Lights::GetLightInfo(size_t light_index) const
{
    auto search = lightIndexToLightInfo_umap.find(light_index);
    assert(search != lightIndexToLightInfo_umap.end());

    return search->second;
}
