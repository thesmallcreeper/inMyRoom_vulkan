#include "Graphics/Exposure.h"
#include "Graphics/Graphics.h"

#include <cmath>

Exposure::Exposure(vk::Device in_device,
                   vma::Allocator in_allocator,
                   const Graphics* in_graphics_ptr,
                   std::tuple<vk::Image, vk::ImageView, vk::ImageCreateInfo> *images_ptr,
                   std::pair<vk::Queue, uint32_t> in_queue,
                   bool in_check_alpha)
    :device(in_device),
     vma_allocator(in_allocator),
     graphics_ptr(in_graphics_ptr),
     queue_family_index(in_queue.second),
     waveSize(graphics_ptr->GetSubgroupSize()),
     localSize(1024),
     checkAlpha(in_check_alpha)
{
    images[0] = images_ptr[0];
    images[1] = images_ptr[1];

    InitBuffers();
    InitDescriptors();
    InitPipeline();
}

Exposure::~Exposure()
{
    vma_allocator.destroyBuffer(resultBuffer, resultAllocation);

    device.destroy(descriptorPool);
    device.destroy(descriptorSetLayout);
}

void Exposure::InitBuffers()
{
    resultBufferRangeSize = sizeof(Histogram);
    resultBufferRangeSize += (resultBufferRangeSize % 16 == 0) ? 0 : 16 - resultBufferRangeSize % 16;

    vk::BufferCreateInfo buffer_create_info;
    buffer_create_info.size = resultBufferRangeSize * 3;
    buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferSrc;
    buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo buffer_allocation_create_info;
    buffer_allocation_create_info.usage = vma::MemoryUsage::eGpuToCpu;
    buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

    auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                          buffer_allocation_create_info,
                                                          resultAllocInfo);

    assert(createBuffer_result.result == vk::Result::eSuccess);
    resultBuffer = createBuffer_result.value.first;
    resultAllocation = createBuffer_result.value.second;
}

void Exposure::InitDescriptors()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 3);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageImage, 3);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 3,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // Storage buffer
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 0;
            attach_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
        }
        {   // Photometric result
            vk::DescriptorSetLayoutBinding attach_binding;
            attach_binding.binding = 1;
            attach_binding.descriptorType = vk::DescriptorType::eStorageImage;
            attach_binding.descriptorCount = 1;
            attach_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;

            bindings.emplace_back(attach_binding);
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

    {   // Write descriptors of storage buffer
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;

        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;
        for (size_t i = 0; i != 3; ++i) {
            auto descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            descriptor_buffer_info_uptr->buffer = resultBuffer;
            descriptor_buffer_info_uptr->offset = i * resultBufferRangeSize;
            descriptor_buffer_info_uptr->range = resultBufferRangeSize;

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

        device.updateDescriptorSets(writes_descriptor_set, {});
    }
}

void Exposure::InitPipeline()
{
    { // Create pipeline layout
        vk::PipelineLayoutCreateInfo pipeline_layout_create_info;

        std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
        std::vector<vk::PushConstantRange> push_constant_range;
        descriptor_sets_layouts.emplace_back(descriptorSetLayout);
        push_constant_range.emplace_back(vk::ShaderStageFlagBits::eCompute, 0, uint32_t(sizeof(ExposureComputePushConstants)));

        pipeline_layout_create_info.setSetLayouts(descriptor_sets_layouts);
        pipeline_layout_create_info.setPushConstantRanges(push_constant_range);

        histogramPipelineLayout = graphics_ptr->GetPipelineFactory()->GetPipelineLayout(pipeline_layout_create_info).first;
    }

    { // Create pipeline
        std::vector<std::pair<std::string, std::string>> definitionStringPairs;
        definitionStringPairs.emplace_back("WAVE_SIZE", std::to_string(waveSize));
        definitionStringPairs.emplace_back("LOCAL_SIZE_X", std::to_string(localSize));
        if (checkAlpha)
            definitionStringPairs.emplace_back("CHECK_ALPHA", "");
        if (std::get<2>(images[0]).samples != vk::SampleCountFlagBits::e1)
            definitionStringPairs.emplace_back("MULTISAMPLED_INPUT", vk::to_string(std::get<2>(images[0]).samples));

        ShadersSpecs shaders_specs = {"Histogram Shader", definitionStringPairs};
        ShadersSet shader_set = graphics_ptr->GetShadersSetsFamiliesCache()->GetShadersSet(shaders_specs);

        vk::PipelineShaderStageRequiredSubgroupSizeCreateInfoEXT require_stable_subgroup_size;
        require_stable_subgroup_size.requiredSubgroupSize = waveSize;

        vk::ComputePipelineCreateInfo compute_pipeline_create_info;
        compute_pipeline_create_info.stage.pNext = &require_stable_subgroup_size;
        compute_pipeline_create_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
        compute_pipeline_create_info.stage.module = shader_set.computeShaderModule;
        compute_pipeline_create_info.stage.pName = "main";
        compute_pipeline_create_info.layout = histogramPipelineLayout;

        histogramPipeline = graphics_ptr->GetPipelineFactory()->GetPipeline(compute_pipeline_create_info).first;
    }
}

void Exposure::CalcNextFrameValue(size_t frame_count, float time_step)
{
    frameCount = frame_count;

    if (frameCount < 4)
        return;

    uint32_t result_buffer_index = frameCount % 3;

    Histogram histogram_local = {};
    memcpy(&histogram_local,
           (std::byte *) (resultAllocInfo.pMappedData) + result_buffer_index * resultBufferRangeSize,
           resultBufferRangeSize);

    float histogram_result = CalculateHistogram(histogram_local);
    currentExposure = histogram_result - std::pow(float(std::numbers::e), - time_step / t63percent) * (histogram_result - currentExposure);
}

float Exposure::CalculateHistogram(Histogram histogram) const
{
    constexpr size_t buckets_count = HISTOGRAM_BUCKETS_COUNT;

    size_t total_sum_uint = size_t(std::get<2>(images[0]).extent.width) * size_t(std::get<2>(images[0]).extent.height);
    auto total_sum = double(total_sum_uint);

    auto low_robustness_zeroings = double(lowRobustness) * total_sum;
    auto high_robustness_zeroings = double(1.f - highRobustness) * total_sum;
    auto population_of_samples = total_sum - low_robustness_zeroings - high_robustness_zeroings;
    auto weight_of_samples= 1. / population_of_samples;
    assert(population_of_samples > 0.);

    // Zero lows
    for (size_t i = 0; i != buckets_count; ++i) {
        if (low_robustness_zeroings > histogram.buckets[i]) {
            low_robustness_zeroings -= histogram.buckets[i];
            histogram.buckets[i] = 0.f;
        } else if (low_robustness_zeroings > 0.) {
            histogram.buckets[i] -= float(low_robustness_zeroings);
            low_robustness_zeroings = 0.;
        }
    }

    // Zero highs
    for (size_t i = buckets_count - 1; i != size_t(-1); --i) {
        if (high_robustness_zeroings > histogram.buckets[i]) {
            high_robustness_zeroings -= histogram.buckets[i];
            histogram.buckets[i] = 0.f;
        } else if (high_robustness_zeroings > 0.) {
            histogram.buckets[i] -= float(high_robustness_zeroings);
            high_robustness_zeroings = 0.;
        }
    }

    // Average logy (geometric mean)
    double min_luminance_log2 = std::log2(double(minLuminance));
    double max_luminance_log2 = std::log2(double(maxLuminance));

    double illuminance_average_log2 = 0.;
    for (size_t i = 0; i != buckets_count; ++i) {
        double lerp_weight = double(i) / double(buckets_count - 1);
        double lerp_result = std::lerp(min_luminance_log2, max_luminance_log2, lerp_weight);
        illuminance_average_log2 += double(histogram.buckets[i]) * lerp_result * weight_of_samples;
    }

    return float(std::pow(2., illuminance_average_log2));
}

void Exposure::RecordFrameHistogram(vk::CommandBuffer command_buffer, uint32_t image_index, uint32_t frames_sum) const
{
    uint32_t hostVisible_buffer_index = frameCount % 3;

    { // Bind image to descriptor set
        vk::WriteDescriptorSet write_descriptor_set;
        vk::DescriptorImageInfo descriptor_image_info;

        descriptor_image_info.imageLayout = vk::ImageLayout::eGeneral;
        descriptor_image_info.imageView = std::get<1>(images[image_index]);
        descriptor_image_info.sampler = nullptr;

        write_descriptor_set.dstSet = descriptorSets[hostVisible_buffer_index];
        write_descriptor_set.dstBinding = 1;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = vk::DescriptorType::eStorageImage;
        write_descriptor_set.pImageInfo = &descriptor_image_info;

        device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);
    }

    { // Dispatch compute
        std::vector<vk::DescriptorSet> descriptor_sets;
        descriptor_sets.emplace_back(descriptorSets[hostVisible_buffer_index]);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute, histogramPipelineLayout, 0, descriptor_sets, {});

        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute, histogramPipeline);

        ExposureComputePushConstants push_constants = {};
        push_constants.size_x = std::get<2>(images[0]).extent.width;
        push_constants.size_y = std::get<2>(images[0]).extent.height;
        push_constants.frame_count = frames_sum;
        push_constants.min_luminance_log2 = std::log2f(minLuminance);
        push_constants.max_luminance_log2 = std::log2f(maxLuminance);

        command_buffer.pushConstants(histogramPipelineLayout, vk::ShaderStageFlagBits::eCompute, 0, sizeof(ExposureComputePushConstants), &push_constants);

        command_buffer.dispatch(1, 1, 1);
    }

    { // Host visible barrier
        vk::BufferMemoryBarrier hostvisible_memory_barriers;
        hostvisible_memory_barriers.srcAccessMask = vk::AccessFlagBits::eShaderWrite;
        hostvisible_memory_barriers.dstAccessMask = vk::AccessFlagBits::eHostRead;
        hostvisible_memory_barriers.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostvisible_memory_barriers.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        hostvisible_memory_barriers.buffer = resultBuffer;
        hostvisible_memory_barriers.offset = hostVisible_buffer_index * resultBufferRangeSize;
        hostvisible_memory_barriers.size = resultBufferRangeSize;

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                       vk::PipelineStageFlagBits::eHost,
                                       vk::DependencyFlagBits::eByRegion,
                                       0, nullptr,
                                       1, &hostvisible_memory_barriers,
                                       0, nullptr);

    }
}

vk::ImageMemoryBarrier Exposure::GetGenericImageBarrier(uint32_t image_index) const {
    vk::ImageMemoryBarrier image_memory_barrier;
    image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
    image_memory_barrier.oldLayout = vk::ImageLayout::eGeneral;
    image_memory_barrier.newLayout = vk::ImageLayout::eGeneral;
    image_memory_barrier.srcQueueFamilyIndex = queue_family_index;
    image_memory_barrier.dstQueueFamilyIndex = queue_family_index;
    image_memory_barrier.image = std::get<0>(images[image_index]);
    image_memory_barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    image_memory_barrier.subresourceRange.baseArrayLayer = 0;
    image_memory_barrier.subresourceRange.baseMipLevel = 0;
    image_memory_barrier.subresourceRange.levelCount = 1;
    image_memory_barrier.subresourceRange.layerCount = 1;

    return image_memory_barrier;
}

void Exposure::ObtainImageOwnership(vk::CommandBuffer command_buffer,
                                    uint32_t image_index,
                                    vk::ImageLayout source_image_layout,
                                    uint32_t source_family_index) const
{
    if (queue_family_index == source_family_index)
        return;

    vk::ImageMemoryBarrier image_memory_barrier = GetGenericImageBarrier(image_index);
    image_memory_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
    image_memory_barrier.oldLayout = source_image_layout;
    image_memory_barrier.srcQueueFamilyIndex = source_family_index;

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                   vk::PipelineStageFlagBits::eComputeShader,
                                   vk::DependencyFlagBits::eByRegion,
                                   0, nullptr,
                                   0, nullptr,
                                   1, &image_memory_barrier);
}

void Exposure::TransferImageOwnership(vk::CommandBuffer command_buffer,
                                      uint32_t image_index,
                                      vk::ImageLayout dst_image_layout,
                                      uint32_t dst_family_index) const
{
    if (queue_family_index == dst_family_index)
        return;

    vk::ImageMemoryBarrier image_memory_barrier = GetGenericImageBarrier(image_index);
    image_memory_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
    image_memory_barrier.newLayout = dst_image_layout;
    image_memory_barrier.dstQueueFamilyIndex = dst_family_index;

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                   vk::PipelineStageFlagBits::eBottomOfPipe,
                                   vk::DependencyFlagBits::eByRegion,
                                   0, nullptr,
                                   0, nullptr,
                                   1, &image_memory_barrier);
}
