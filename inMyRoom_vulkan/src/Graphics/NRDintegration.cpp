#include "Graphics/NRDintegration.h"

#include <vector>

NRDintegration::NRDstaticSamplers::NRDstaticSamplers(vk::Device vk_device)
    :device(vk_device)
{
}

NRDintegration::NRDstaticSamplers::~NRDstaticSamplers()
{
    for(auto it : nrdSamplerToVKsampler_umap) {
        device.destroy(it.second);
    }
}

vk::Sampler* NRDintegration::NRDstaticSamplers::GetVKsampler(nrd::Sampler nrd_sampler)
{
    auto search = nrdSamplerToVKsampler_umap.find(nrd_sampler);
    if (search != nrdSamplerToVKsampler_umap.end()) {
        return &search->second;
    } else {
        vk::SamplerAddressMode address_mode;
        if (nrd_sampler == nrd::Sampler::LINEAR_CLAMP || nrd_sampler == nrd::Sampler::NEAREST_CLAMP)
            address_mode = vk::SamplerAddressMode::eClampToEdge;
        else
            address_mode = vk::SamplerAddressMode::eMirroredRepeat;

        vk::Filter filter;
        if (nrd_sampler == nrd::Sampler::LINEAR_CLAMP || nrd_sampler == nrd::Sampler::LINEAR_MIRRORED_REPEAT)
            filter = vk::Filter::eLinear;
        else
            filter = vk::Filter::eNearest;

        vk::SamplerCreateInfo sampler_create_info;
        sampler_create_info.magFilter = filter;
        sampler_create_info.minFilter = filter;
        sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eNearest;
        sampler_create_info.addressModeU = address_mode;
        sampler_create_info.addressModeV = address_mode;
        sampler_create_info.mipLodBias = 0.f;
        sampler_create_info.anisotropyEnable = VK_FALSE;
        sampler_create_info.maxAnisotropy = 0.f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = vk::CompareOp::eAlways;
        sampler_create_info.minLod = 0.f;
        sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
        sampler_create_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        vk::Sampler sampler = device.createSampler(sampler_create_info).value;
        auto result = nrdSamplerToVKsampler_umap.emplace(nrd_sampler, sampler);

        return &result.first->second;
    }
}


NRDintegration::NRDtextureWrapper::NRDtextureWrapper(vk::Device in_device, vma::Allocator in_allocator,
                                                     nrd::Format nrd_format, uint32_t width, uint32_t height, uint32_t mipLevels)
        : device(in_device),
          vma_allocator(in_allocator),
          wrapperImageOwner(true),
          isValidTexture(true)
{
    imageInfo.imageType = vk::ImageType::e2D;
    imageInfo.format = NRDtoVKformat(nrd_format);
    imageInfo.extent = vk::Extent3D(width, height, 1);
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.samples = vk::SampleCountFlagBits::e1;
    imageInfo.sharingMode = vk::SharingMode::eExclusive;
    imageInfo.tiling = vk::ImageTiling::eOptimal;
    imageInfo.usage = vk::ImageUsageFlagBits::eStorage | vk::ImageUsageFlagBits::eSampled;
    imageInfo.initialLayout = vk::ImageLayout::eUndefined;

    vma::AllocationCreateInfo image_allocation_info;
    image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

    auto createImage_result = vma_allocator.createImage(imageInfo, image_allocation_info).value;
    image = createImage_result.first;
    imageAllocation = createImage_result.second;

    mipLayouts.resize(imageInfo.mipLevels, vk::ImageLayout::eUndefined);
}

NRDintegration::NRDtextureWrapper::NRDtextureWrapper(vk::Device in_device, vk::Image in_image, vk::ImageCreateInfo in_image_info)
        : device(in_device),
          isValidTexture(true)
{
    image = in_image;
    imageInfo = in_image_info;
    mipLayouts.resize(imageInfo.mipLevels, vk::ImageLayout::eUndefined);
}

NRDintegration::NRDtextureWrapper::~NRDtextureWrapper()
{
    Deinit();
}

void NRDintegration::NRDtextureWrapper::Deinit()
{
    if (isValidTexture) {
        for (auto &this_pair: mipOffsetCountPairToImageView_umap) {
            device.destroy(this_pair.second);
        }

        if (wrapperImageOwner) {
            vma_allocator.destroyImage(image, imageAllocation);
        }
    }

    isValidTexture = false;
}

NRDintegration::NRDtextureWrapper::NRDtextureWrapper(NRDtextureWrapper&& other)  noexcept
{
    *this = std::move(other);
}

NRDintegration::NRDtextureWrapper &NRDintegration::NRDtextureWrapper::operator=(NRDintegration::NRDtextureWrapper &&other) noexcept
{
    // Destruct
    this->Deinit();

    image = other.image;
    imageInfo = other.imageInfo;
    mipLayouts = std::move(other.mipLayouts);

    mipOffsetCountPairToImageView_umap = std::move( other.mipOffsetCountPairToImageView_umap);

    wrapperImageOwner = other.wrapperImageOwner;
    imageAllocation = other.imageAllocation;
    device = other.device;
    vma_allocator = other.vma_allocator;

    isValidTexture = other.isValidTexture;
    other.isValidTexture = false;

    return *this;
}

std::vector<vk::ImageMemoryBarrier> NRDintegration::NRDtextureWrapper::SetLayout(vk::ImageLayout dst_layout, uint32_t mip_offset, uint32_t mip_count)
{
    assert(mip_count != 0);
    assert(dst_layout == vk::ImageLayout::eGeneral
    || dst_layout == vk::ImageLayout::eShaderReadOnlyOptimal
    || dst_layout == vk::ImageLayout::eUndefined);

    std::vector<vk::ImageMemoryBarrier> return_vector;

    mip_count = std::min(imageInfo.mipLevels - mip_offset, mip_count);
    assert(mip_count != 0);

    for (uint32_t mip = mip_offset; mip != mip_offset + mip_count; ++mip) {
        vk::ImageLayout mip_layout = mipLayouts[mip];
        if (mip_layout != dst_layout) {
            vk::ImageMemoryBarrier mip_image_barrier;
            if (mip_layout == vk::ImageLayout::eGeneral)
                mip_image_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
            else if (mip_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
                mip_image_barrier.srcAccessMask = vk::AccessFlagBits::eShaderRead;
            else
                mip_image_barrier.srcAccessMask = vk::AccessFlagBits::eNone;
            mip_image_barrier.oldLayout = mip_layout;

            if (dst_layout == vk::ImageLayout::eGeneral)
                mip_image_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead | vk::AccessFlagBits::eShaderWrite;
            else if (dst_layout == vk::ImageLayout::eShaderReadOnlyOptimal)
                mip_image_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
            mip_image_barrier.newLayout = dst_layout;

            mip_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            mip_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            mip_image_barrier.image = image;
            mip_image_barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  mip, 1,
                                                  0, 1};
            return_vector.emplace_back(mip_image_barrier);
            mipLayouts[mip] = dst_layout;
        }
    }

    return return_vector;
}

vk::DescriptorImageInfo NRDintegration::NRDtextureWrapper::GetDescriptorImageInfo(uint32_t mip_offset, uint32_t mip_count)
{
    vk::DescriptorImageInfo descriptor_image_info;
    descriptor_image_info.sampler = nullptr;
    descriptor_image_info.imageView = GetImageView(mip_offset, mip_count);
    descriptor_image_info.imageLayout = mipLayouts[mip_offset];

    return descriptor_image_info;
}

vk::ImageView NRDintegration::NRDtextureWrapper::GetImageView(uint32_t mip_offset, uint32_t mip_count)
{
    uint64_t key = (uint64_t)(mip_offset) << 32 | (uint64_t)(mip_count);
    auto search = mipOffsetCountPairToImageView_umap.find(key);
    if (search != mipOffsetCountPairToImageView_umap.end()) {
        return search->second;
    } else {
        vk::ImageViewCreateInfo imageView_create_info;
        imageView_create_info.image = image;
        imageView_create_info.viewType = vk::ImageViewType::e2D;
        imageView_create_info.format = imageInfo.format;
        imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity,
                                            vk::ComponentSwizzle::eIdentity};
        imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                  mip_offset, mip_count,
                                                  0, 1};

        vk::ImageView image_view = device.createImageView(imageView_create_info).value;

        mipOffsetCountPairToImageView_umap.emplace(key, image_view);
        return image_view;
    }
}

vk::Format NRDintegration::NRDtextureWrapper::NRDtoVKformat(nrd::Format nrd_format)
{
    vk::Format nrd_to_vk_table[] =
    {
        vk::Format::eR8Unorm,
        vk::Format::eR8Snorm,
        vk::Format::eR8Uint,
        vk::Format::eR8Sint,
        vk::Format::eR8G8Unorm,
        vk::Format::eR8G8Snorm,
        vk::Format::eR8G8Uint,
        vk::Format::eR8G8Sint,
        vk::Format::eR8G8B8A8Unorm,
        vk::Format::eR8G8B8A8Snorm,
        vk::Format::eR8G8B8A8Uint,
        vk::Format::eR8G8B8A8Sint,
        vk::Format::eR8G8B8A8Srgb,
        vk::Format::eR16Unorm,
        vk::Format::eR16Snorm,
        vk::Format::eR16Uint,
        vk::Format::eR16Sint,
        vk::Format::eR16Sfloat,
        vk::Format::eR16G16Unorm,
        vk::Format::eR16G16Snorm,
        vk::Format::eR16G16Uint,
        vk::Format::eR16G16Sint,
        vk::Format::eR16G16Sfloat,
        vk::Format::eR16G16B16A16Unorm,
        vk::Format::eR16G16B16A16Snorm,
        vk::Format::eR16G16B16A16Uint,
        vk::Format::eR16G16B16A16Sint,
        vk::Format::eR16G16B16A16Sfloat,
        vk::Format::eR32Uint,
        vk::Format::eR32Sint,
        vk::Format::eR32Sfloat,
        vk::Format::eR32G32Uint,
        vk::Format::eR32G32Sint,
        vk::Format::eR32G32Sfloat,
        vk::Format::eR32G32B32Uint,
        vk::Format::eR32G32B32Sint,
        vk::Format::eR32G32B32Sfloat,
        vk::Format::eR32G32B32A32Uint,
        vk::Format::eR32G32B32A32Sint,
        vk::Format::eR32G32B32A32Sfloat,
        vk::Format::eA2R10G10B10UnormPack32,
        vk::Format::eA2R10G10B10UintPack32,
        vk::Format::eB10G11R11UfloatPack32,
        vk::Format::eE5B9G9R9UfloatPack32,
    };

    return nrd_to_vk_table[(size_t)nrd_format];
}

NRDintegration::NRDintegration(vk::Device in_device,
                               vma::Allocator in_allocator,
                               PipelinesFactory* in_pipelinesFactory_ptr,
                               const nrd::DenoiserCreationDesc &denoiserCreationDesc)
    :device(in_device),
     vma_allocator(in_allocator),
     pipelinesFactory_ptr(in_pipelinesFactory_ptr),
     staticSamplers(in_device)
{
    Initialize(denoiserCreationDesc);
}

NRDintegration::~NRDintegration()
{
    device.destroy(descriptorPool);

    for(size_t i = 0; i != pipelines.size(); i++) {
        device.destroy(modules[i]);
        device.destroy(descriptorSetLayouts[i]);
    }

    if (NRDdenoiser_ptr) {
        nrd::DestroyDenoiser(*NRDdenoiser_ptr);
        NRDdenoiser_ptr = nullptr;
    }

    vma_allocator.destroyBuffer(constantBuffer, constantBufferAllocation);

    // deletion of staticSamplers
    // deletion of privateTexturePool
}

void NRDintegration::Initialize(const nrd::DenoiserCreationDesc &denoiserCreationDesc)
{
    auto result = nrd::CreateDenoiser(denoiserCreationDesc, NRDdenoiser_ptr);
    assert(result == nrd::Result::SUCCESS);

    CreateLayoutsAndPipelines();
    CreateDescriptorSets();
    CreateTexturesBuffers();
}

void NRDintegration::CreateLayoutsAndPipelines()
{
    const nrd::DenoiserDesc& denoiser_desc = nrd::GetDenoiserDesc(*NRDdenoiser_ptr);
    const nrd::SPIRVBindingOffsets& binding_offsets = nrd::GetLibraryDesc().spirvBindingOffsets;

    for (uint32_t i = 0; i < denoiser_desc.pipelineNum; i++)
    {
        std::vector<vk::DescriptorSetLayoutBinding> layout_bindings;
        const nrd::PipelineDesc& pipeline_desc = denoiser_desc.pipelines[i];

        // Add samplers bindings
        for (uint32_t j = 0; j < denoiser_desc.staticSamplerNum; j++)
        {
            const nrd::StaticSamplerDesc& nrd_sampler = denoiser_desc.staticSamplers[j];
            vk::Sampler* vk_sampler_ptr = staticSamplers.GetVKsampler(nrd_sampler.sampler);

            vk::DescriptorSetLayoutBinding sampler_layout_binding;
            sampler_layout_binding.binding = binding_offsets.samplerOffset + nrd_sampler.registerIndex;
            sampler_layout_binding.descriptorCount = 1;
            sampler_layout_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;
            sampler_layout_binding.descriptorType = vk::DescriptorType::eSampler;
            sampler_layout_binding.pImmutableSamplers = vk_sampler_ptr;

            layout_bindings.emplace_back(sampler_layout_binding);
        }

        // Add textures and storage textures binding
        for (uint32_t j = 0; j < pipeline_desc.descriptorRangeNum; j++)
        {
            const nrd::DescriptorRangeDesc& descriptor_range = pipeline_desc.descriptorRanges[j];

            vk::DescriptorSetLayoutBinding common_texture_layout_binding;
            common_texture_layout_binding.descriptorCount = 1;
            common_texture_layout_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;
            if (descriptor_range.descriptorType == nrd::DescriptorType::TEXTURE)
            {
                common_texture_layout_binding.descriptorType = vk::DescriptorType::eSampledImage;
                common_texture_layout_binding.binding = binding_offsets.textureOffset + descriptor_range.baseRegisterIndex;
            } else {
                common_texture_layout_binding.descriptorType = vk::DescriptorType::eStorageImage;
                common_texture_layout_binding.binding = binding_offsets.storageTextureAndBufferOffset + descriptor_range.baseRegisterIndex;
            }

            for (uint32_t k = 0; k < descriptor_range.descriptorNum; k++)
            {
                vk::DescriptorSetLayoutBinding this_texture_layout_binding = common_texture_layout_binding;
                this_texture_layout_binding.binding += k;

                layout_bindings.emplace_back(this_texture_layout_binding);
            }
        }

        // Add constant data bindings
        if (pipeline_desc.hasConstantData) {
            vk::DescriptorSetLayoutBinding constant_buffer_layout_binding;
            constant_buffer_layout_binding.binding = binding_offsets.constantBufferOffset;
            constant_buffer_layout_binding.descriptorCount = 1;
            constant_buffer_layout_binding.stageFlags = vk::ShaderStageFlagBits::eCompute;
            constant_buffer_layout_binding.descriptorType = vk::DescriptorType::eUniformBuffer;

            layout_bindings.emplace_back(constant_buffer_layout_binding);
        }

        // Create descriptor set layout
        vk::DescriptorSetLayoutCreateInfo descriptorSet_layout_info;
        descriptorSet_layout_info.setBindings(layout_bindings);

        vk::DescriptorSetLayout descriptorSet_layout = device.createDescriptorSetLayout(descriptorSet_layout_info).value;

        // Create pipeline layout
        vk::PipelineLayoutCreateInfo pipeline_layout_info;
        pipeline_layout_info.setLayoutCount = 1;
        pipeline_layout_info.pSetLayouts = &descriptorSet_layout;

        vk::PipelineLayout pipeline_layout = pipelinesFactory_ptr->GetPipelineLayout(pipeline_layout_info).first;

        // Create shader module
        vk::ShaderModuleCreateInfo module_info;
        module_info.codeSize = pipeline_desc.computeShaderSPIRV.size;
        module_info.pCode = (uint32_t*)(pipeline_desc.computeShaderSPIRV.bytecode);

        vk::ShaderModule module = device.createShaderModule(module_info).value;

        // Create pipelines
        vk::ComputePipelineCreateInfo pipeline_info;
        pipeline_info.stage.stage = vk::ShaderStageFlagBits::eCompute;
        pipeline_info.stage.module = module;
        pipeline_info.stage.pName = "main";
        pipeline_info.layout = pipeline_layout;

        vk::Pipeline pipeline = pipelinesFactory_ptr->GetPipeline(pipeline_info).first;

        // Push-back
        descriptorSetLayouts.emplace_back(descriptorSet_layout);
        pipelineLayouts.emplace_back(pipeline_layout);
        modules.emplace_back(module);
        pipelines.emplace_back(pipeline);
    }
}

void NRDintegration::CreateDescriptorSets()
{
    const nrd::DenoiserDesc& denoiser_desc = nrd::GetDenoiserDesc(*NRDdenoiser_ptr);

    // Create descriptor pool
    std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
    descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageImage , 3 * denoiser_desc.descriptorSetDesc.storageTextureMaxNum);
    descriptor_pool_sizes.emplace_back(vk::DescriptorType::eSampledImage , 3 * denoiser_desc.descriptorSetDesc.textureMaxNum);
    descriptor_pool_sizes.emplace_back(vk::DescriptorType::eUniformBuffer, 3 * denoiser_desc.descriptorSetDesc.constantBufferMaxNum);
    descriptor_pool_sizes.emplace_back(vk::DescriptorType::eSampler      , 3 * denoiser_desc.descriptorSetDesc.staticSamplerMaxNum);
    vk::DescriptorPoolCreateInfo descriptor_pool_create_info(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 3 * denoiser_desc.descriptorSetDesc.setMaxNum,
                                                             descriptor_pool_sizes);

    descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
}

void NRDintegration::CreateTexturesBuffers()
{
    const nrd::DenoiserDesc& denoiser_desc = nrd::GetDenoiserDesc(*NRDdenoiser_ptr);

    // Create texture pool
    for (size_t i = 0; i != denoiser_desc.permanentPoolSize; ++i) {
        const nrd::TextureDesc& texture_desc = denoiser_desc.permanentPool[i];
        privateTexturePool.emplace_back(device, vma_allocator,
                                        texture_desc.format, texture_desc.width, texture_desc.height, texture_desc.mipNum);

    }
    for (size_t i = 0; i != denoiser_desc.transientPoolSize; ++i) {
        const nrd::TextureDesc& texture_desc = denoiser_desc.transientPool[i];
        privateTexturePool.emplace_back(device, vma_allocator,
                                        texture_desc.format, texture_desc.width, texture_desc.height, texture_desc.mipNum);

    }

    // Create constant buffer
    constantBufferPerSetSize = ((denoiser_desc.constantBufferDesc.maxDataSize + 64 - 1) / 64) * 64;
    constantBufferRangeSize = denoiser_desc.descriptorSetDesc.setMaxNum * constantBufferPerSetSize;

    vk::BufferCreateInfo buffer_create_info;
    buffer_create_info.size = constantBufferRangeSize * 3;
    buffer_create_info.usage = vk::BufferUsageFlagBits::eUniformBuffer | vk::BufferUsageFlagBits::eTransferDst;
    buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

    vma::AllocationCreateInfo buffer_allocation_create_info;
    buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
    buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

    auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                          buffer_allocation_create_info,
                                                          constantBufferAllocationInfo);
    assert(createBuffer_result.result == vk::Result::eSuccess);
    constantBuffer = createBuffer_result.value.first;
    constantBufferAllocation = createBuffer_result.value.second;
}

void NRDintegration::BindTexture(nrd::ResourceType nrd_resource_type, vk::Image image, vk::ImageCreateInfo image_info,
                                 vk::ImageLayout initial_layout, vk::ImageLayout final_layout)
{
    assert((uint32_t)nrd_resource_type < (uint32_t)nrd::ResourceType::MAX_NUM - 2);

    userTexturePool[(size_t)nrd_resource_type] = std::move(NRDtextureWrapper(device, image, image_info));
    userTexturePoolInitialLayout[(size_t)nrd_resource_type] = initial_layout;
    userTexturePoolFinalLayout[(size_t)nrd_resource_type] = final_layout;
}

void NRDintegration::SetMethodSettings(nrd::Method method, const void *methodSettings)
{
    assert(NRDdenoiser_ptr);

    nrd::Result result = nrd::SetMethodSettings(*NRDdenoiser_ptr, method, methodSettings);
    assert(result == nrd::Result::SUCCESS);
}

void NRDintegration::PrepareNewFrame(size_t frame_index, const nrd::CommonSettings& commonSettings)
{
    frameIndex = frame_index;
    uint32_t buffer_index = (frameIndex % 3);

    const nrd::DenoiserDesc& denoiser_desc = nrd::GetDenoiserDesc(*NRDdenoiser_ptr);
    const nrd::SPIRVBindingOffsets& binding_offsets = nrd::GetLibraryDesc().spirvBindingOffsets;

    // Set state of user pool textures
    for (size_t i = 0; i != userTexturePool.size(); ++i) {
        if (userTexturePool[i].IsValidTexture()) {
            userTexturePool[i].SetLayout(userTexturePoolInitialLayout[i]);
        }
    }

    // TODO: undefined for transient textures?

    // Get denoiser dispatches
    nrd::Result result = nrd::GetComputeDispatches(*NRDdenoiser_ptr, commonSettings, NRDdispatches_ptr, NRDdispatches_count);
    assert(result == nrd::Result::SUCCESS);

    // Delete previous image barries per dispatch
    imageBarriersPerDispatch.clear();

    // Update constant buffer
    for (size_t i = 0; i != NRDdispatches_count; ++i) {
        const nrd::DispatchDesc& this_dispatch = NRDdispatches_ptr[i];
        if (this_dispatch.constantBufferDataSize)
        {
            memcpy((std::byte*)constantBufferAllocationInfo.pMappedData + buffer_index * constantBufferRangeSize + i * constantBufferPerSetSize,
                   this_dispatch.constantBufferData,
                   this_dispatch.constantBufferDataSize);
        }
    }
    vma_allocator.flushAllocation(constantBufferAllocation, buffer_index * constantBufferRangeSize, constantBufferRangeSize);

    // Free old descriptor sets
    std::vector<vk::DescriptorSet> descriptor_sets_to_free;
    for(auto& this_set: descriptorSets[buffer_index]) {
        descriptor_sets_to_free.emplace_back(this_set);
    }
    if (descriptor_sets_to_free.size()) {
        device.freeDescriptorSets(descriptorPool, descriptor_sets_to_free);
    }

    // Allocate descriptor sets (one for each dispatch)
    std::vector<vk::DescriptorSetLayout> descriptor_sets_layouts;
    for (size_t i = 0; i != NRDdispatches_count; ++i) {
        const nrd::DispatchDesc& this_dispatch = NRDdispatches_ptr[i];
        vk::DescriptorSetLayout this_descriptor_set_layout = descriptorSetLayouts[this_dispatch.pipelineIndex];
        descriptor_sets_layouts.emplace_back(this_descriptor_set_layout);
    }
    vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, descriptor_sets_layouts);
    descriptorSets[buffer_index] = device.allocateDescriptorSets(descriptor_set_allocate_info).value;

    // Write descriptor sets and gather image barriers
    std::vector<vk::WriteDescriptorSet> descriptor_sets_writes;
    std::vector<std::unique_ptr<vk::DescriptorImageInfo>> descriptor_image_infos;
    std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffers_infos;
    for (size_t i = 0; i != NRDdispatches_count; ++i) {
        const nrd::DispatchDesc& this_dispatch = NRDdispatches_ptr[i];
        const nrd::PipelineDesc& pipeline_desc = denoiser_desc.pipelines[this_dispatch.pipelineIndex];

        // For descriptor textures
        size_t resource_index = 0;
        std::vector<vk::ImageMemoryBarrier> this_dispatch_image_barriers;
        for (size_t j = 0; j != pipeline_desc.descriptorRangeNum; ++j) {
            const nrd::DescriptorRangeDesc& this_desc_range = pipeline_desc.descriptorRanges[j];

            for (size_t k = 0; k != this_desc_range.descriptorNum; ++k) {
                const nrd::Resource& this_resource = this_dispatch.resources[resource_index++];

                // Get texture
                NRDtextureWrapper* this_texture_ptr = nullptr;
                if (this_resource.type == nrd::ResourceType::PERMANENT_POOL)
                    this_texture_ptr = &privateTexturePool[this_resource.indexInPool];
                else if (this_resource.type == nrd::ResourceType::TRANSIENT_POOL)
                    this_texture_ptr = &privateTexturePool[this_resource.indexInPool + denoiser_desc.permanentPoolSize];
                else
                    this_texture_ptr = &userTexturePool[(size_t)this_resource.type];
                assert(this_texture_ptr->IsValidTexture());
                assert(this_resource.stateNeeded == this_desc_range.descriptorType);

                // Set texture state and get barrier
                vk::ImageLayout layout = (this_resource.stateNeeded == nrd::DescriptorType::TEXTURE)? vk::ImageLayout::eShaderReadOnlyOptimal : vk::ImageLayout::eGeneral;
                auto image_barriers = this_texture_ptr->SetLayout(layout, this_resource.mipOffset, this_resource.mipNum);
                std::copy(image_barriers.begin(), image_barriers.end(), std::back_inserter(this_dispatch_image_barriers));

                // Get texture write
                std::unique_ptr<vk::DescriptorImageInfo> this_descImage_info_uptr = std::make_unique<vk::DescriptorImageInfo>(this_texture_ptr->GetDescriptorImageInfo(this_resource.mipOffset, this_resource.mipNum));
                vk::WriteDescriptorSet this_descriptor_write = {};
                this_descriptor_write.dstSet = descriptorSets[buffer_index][i];
                this_descriptor_write.dstBinding = k + this_desc_range.baseRegisterIndex +
                        ((this_resource.stateNeeded == nrd::DescriptorType::TEXTURE)? binding_offsets.textureOffset : binding_offsets.storageTextureAndBufferOffset);
                this_descriptor_write.dstArrayElement = 0;
                this_descriptor_write.descriptorCount = 1;
                this_descriptor_write.descriptorType = (this_resource.stateNeeded == nrd::DescriptorType::TEXTURE)? vk::DescriptorType::eSampledImage :vk::DescriptorType::eStorageImage;
                this_descriptor_write.pImageInfo = this_descImage_info_uptr.get();

                descriptor_sets_writes.emplace_back(this_descriptor_write);
                descriptor_image_infos.emplace_back(std::move(this_descImage_info_uptr));
            }
        }

        // For constant buffer
        if (pipeline_desc.hasConstantData) {
            std::unique_ptr<vk::DescriptorBufferInfo> buffer_descriptor_info_uptr = std::make_unique<vk::DescriptorBufferInfo>();
            buffer_descriptor_info_uptr->buffer = constantBuffer;
            buffer_descriptor_info_uptr->offset = buffer_index * constantBufferRangeSize + i * constantBufferPerSetSize;
            buffer_descriptor_info_uptr->range = constantBufferPerSetSize;

            vk::WriteDescriptorSet buffer_descriptor_write = {};
            buffer_descriptor_write.dstSet = descriptorSets[buffer_index][i];
            buffer_descriptor_write.dstBinding = binding_offsets.constantBufferOffset;
            buffer_descriptor_write.dstArrayElement = 0;
            buffer_descriptor_write.descriptorCount = 1;
            buffer_descriptor_write.descriptorType = vk::DescriptorType::eUniformBuffer;
            buffer_descriptor_write.pBufferInfo = buffer_descriptor_info_uptr.get();

            descriptor_sets_writes.emplace_back(buffer_descriptor_write);
            descriptor_buffers_infos.emplace_back(std::move(buffer_descriptor_info_uptr));
        }

        // Push-back image barriers
        imageBarriersPerDispatch.emplace_back(this_dispatch_image_barriers);
    }
    device.updateDescriptorSets(descriptor_sets_writes, {});
}

void NRDintegration::Denoise(vk::CommandBuffer command_buffer, vk::PipelineStageFlagBits src_stage, vk::PipelineStageFlagBits dst_stage)
{
    uint32_t buffer_index = (frameIndex % 3);

    // Dispatch
    for (size_t i = 0; i != NRDdispatches_count; ++i) {
        const nrd::DispatchDesc& this_dispatch = NRDdispatches_ptr[i];

        vk::DebugUtilsLabelEXT laber_info;
        laber_info.pLabelName = this_dispatch.name;
        command_buffer.beginDebugUtilsLabelEXT(laber_info);

        command_buffer.pipelineBarrier((i == 0)? src_stage : vk::PipelineStageFlagBits::eComputeShader,
                                       vk::PipelineStageFlagBits::eComputeShader,
                                       vk::DependencyFlagBits::eByRegion,
                                       {},
                                       {},
                                       imageBarriersPerDispatch[i]);
        command_buffer.bindDescriptorSets(vk::PipelineBindPoint::eCompute,
                                          pipelineLayouts[this_dispatch.pipelineIndex],
                                          0,
                                          1, &descriptorSets[buffer_index][i],
                                          0, nullptr);
        command_buffer.bindPipeline(vk::PipelineBindPoint::eCompute,
                                    pipelines[this_dispatch.pipelineIndex]);
        command_buffer.dispatch(this_dispatch.gridWidth,
                                this_dispatch.gridHeight,
                                1);

        command_buffer.endDebugUtilsLabelEXT();
    }

    // Barrier out user texture
    std::vector<vk::ImageMemoryBarrier> final_imageBarriers;
    for (size_t i = 0; i != userTexturePool.size(); ++i) {
        auto& this_texture = userTexturePool[i];
        if (this_texture.IsValidTexture()) {
            auto image_barriers = this_texture.SetLayout(userTexturePoolFinalLayout[i]);
            std::copy(image_barriers.begin(), image_barriers.end(), std::back_inserter(final_imageBarriers));
        }
    }
    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eComputeShader,
                                   dst_stage,
                                   vk::DependencyFlagBits::eByRegion,
                                   {},
                                   {},
                                   final_imageBarriers);
}
