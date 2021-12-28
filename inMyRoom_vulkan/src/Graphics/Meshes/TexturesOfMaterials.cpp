#include "Graphics/Meshes/TexturesOfMaterials.h"

#include "Graphics/HelperUtils.h"

TexturesOfMaterials::TexturesOfMaterials(vk::Device in_device,
                                         vma::Allocator in_vma_allocator,
                                         std::pair<vk::Queue, uint32_t> graphics_queue)
    :device(in_device),
     vma_allocator(in_vma_allocator),
     transferQueue(graphics_queue)
{
}

TexturesOfMaterials::~TexturesOfMaterials()
{
    for(auto& this_pair: samplerSpecToSampler_umap) {
        device.destroy(this_pair.second);
    }
    for(auto& this_pair: textures) {
        device.destroy(this_pair.first);
    }
    for(auto& this_pair: vkImagesAndAllocations) {
        vma_allocator.destroyImage(this_pair.first, this_pair.second);
    }
}

size_t TexturesOfMaterials::AddTextureAndMipmaps(const std::vector<ImageData> &images_data, vk::Format format)
{
    // Create image
    vk::ImageCreateInfo image_create_info;
    image_create_info.imageType = vk::ImageType::e2D;
    image_create_info.format = format;
    image_create_info.extent = vk::Extent3D(uint32_t(images_data[0].GetWidth()), uint32_t(images_data[0].GetHeight()), 1);
    image_create_info.mipLevels = uint32_t(images_data.size());
    image_create_info.arrayLayers = 1;
    image_create_info.samples = vk::SampleCountFlagBits::e1;
    image_create_info.sharingMode = vk::SharingMode::eExclusive;
    image_create_info.tiling = vk::ImageTiling::eOptimal;
    image_create_info.usage = vk::ImageUsageFlagBits::eTransferDst | vk::ImageUsageFlagBits::eSampled;
    image_create_info.initialLayout = vk::ImageLayout::eUndefined;

    vma::AllocationCreateInfo image_allocation_info;
    image_allocation_info.usage = vma::MemoryUsage::eGpuOnly;

    auto createImage_result = vma_allocator.createImage(image_create_info, image_allocation_info).value;
    vk::Image image = createImage_result.first;
    vma::Allocation allocation = createImage_result.second;

    vkImagesAndAllocations.emplace_back(image, allocation);

    // Imageview
    vk::ImageViewCreateInfo imageView_create_info;
    imageView_create_info.image = image;
    imageView_create_info.viewType = vk::ImageViewType::e2D;
    imageView_create_info.format = format;
    imageView_create_info.components = {vk::ComponentSwizzle::eIdentity,
                                        images_data[0].GetComponentsCount() >= 2 ? vk::ComponentSwizzle::eIdentity : vk::ComponentSwizzle::eZero,
                                        images_data[0].GetComponentsCount() >= 3 ? vk::ComponentSwizzle::eIdentity : vk::ComponentSwizzle::eZero,
                                        images_data[0].GetComponentsCount() == 4 ? vk::ComponentSwizzle::eIdentity : vk::ComponentSwizzle::eOne};
    imageView_create_info.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                              0, uint32_t(images_data.size()),
                                              0, 1};

    vk::ImageView imageView = device.createImageView(imageView_create_info).value;

    // Pick sampler
    vk::Sampler sampler = GetSampler({images_data[0].GetWrapS(), images_data[0].GetWrapT()});

    // Transfer data
    std::vector<std::byte> data_of_images;
    std::vector<vk::BufferImageCopy> regions;

    for(size_t i = 0; i != images_data.size(); ++i) {
        std::vector<std::byte> this_image_data = GetImageFromImageData(images_data[i], format);
        vk::BufferImageCopy region = {data_of_images.size(),
                                      0, 0,
                                      {vk::ImageAspectFlagBits::eColor, uint32_t(i), 0, 1},
                                      {0, 0 ,0},
                                      {uint32_t(images_data[i].GetWidth()), uint32_t(images_data[i].GetHeight()), 1}};

        std::copy(this_image_data.begin(), this_image_data.end(), std::back_inserter(data_of_images));
        regions.emplace_back(region);
    }

    StagingBuffer staging_buffer(device, vma_allocator, data_of_images.size());
    memcpy(staging_buffer.GetDstPtr(), data_of_images.data(), data_of_images.size());

    {
        vk::CommandBuffer command_buffer = staging_buffer.BeginCommandRecord(transferQueue);

        vk::ImageMemoryBarrier init_image_barrier;
        init_image_barrier.image = image;
        init_image_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
        init_image_barrier.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        init_image_barrier.oldLayout = vk::ImageLayout::eUndefined;
        init_image_barrier.newLayout = vk::ImageLayout::eTransferDstOptimal;
        init_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        init_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        init_image_barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                 0, uint32_t(images_data.size()), 0, 1};

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eBottomOfPipe,
                                       vk::PipelineStageFlagBits::eTransfer,
                                       vk::DependencyFlagBits::eByRegion,
                                       0, nullptr,
                                       0, nullptr,
                                       1, &init_image_barrier);

        command_buffer.copyBufferToImage(staging_buffer.GetBuffer(), image, vk::ImageLayout::eTransferDstOptimal, regions);

        vk::ImageMemoryBarrier sample_image_barrier;
        sample_image_barrier.image = image;
        sample_image_barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        sample_image_barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        sample_image_barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        sample_image_barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        sample_image_barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sample_image_barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        sample_image_barrier.subresourceRange = {vk::ImageAspectFlagBits::eColor,
                                                 0, uint32_t(images_data.size()), 0, 1};

        command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTransfer,
                                       vk::PipelineStageFlagBits::eFragmentShader,
                                       vk::DependencyFlagBits::eByRegion,
                                       0, nullptr,
                                       0, nullptr,
                                       1, &sample_image_barrier);

        staging_buffer.EndAndSubmitCommands();
    }

    textures.emplace_back(imageView, sampler);
    return textures.size() - 1;
}

vk::Sampler TexturesOfMaterials::GetSampler(SamplerSpecs samplerSpecs)
{
    auto search = samplerSpecToSampler_umap.find(samplerSpecs);
    if (search != samplerSpecToSampler_umap.end()) {
        return search->second;
    } else {
        vk::SamplerAddressMode sampler_vk_wrapS= glTFsamplerWrapToAddressMode_map.find(samplerSpecs.wrap_S)->second;
        vk::SamplerAddressMode sampler_vk_wrapT= glTFsamplerWrapToAddressMode_map.find(samplerSpecs.wrap_T)->second;

        vk::SamplerCreateInfo sampler_create_info;
        sampler_create_info.magFilter = vk::Filter::eLinear;
        sampler_create_info.minFilter = vk::Filter::eLinear;
        sampler_create_info.mipmapMode = vk::SamplerMipmapMode::eLinear;
        sampler_create_info.addressModeU = sampler_vk_wrapS;
        sampler_create_info.addressModeV = sampler_vk_wrapT;
        sampler_create_info.mipLodBias = 0.f;
        sampler_create_info.anisotropyEnable = VK_TRUE;
        sampler_create_info.maxAnisotropy = 16.f;
        sampler_create_info.compareEnable = VK_FALSE;
        sampler_create_info.compareOp = vk::CompareOp::eAlways;
        sampler_create_info.minLod = 0.f;
        sampler_create_info.maxLod = VK_LOD_CLAMP_NONE;
        sampler_create_info.borderColor = vk::BorderColor::eIntOpaqueBlack;
        sampler_create_info.unnormalizedCoordinates = VK_FALSE;

        vk::Sampler sampler = device.createSampler(sampler_create_info).value;
        samplerSpecToSampler_umap.emplace(samplerSpecs, sampler);

        return sampler;
    }
}

std::vector<std::byte> TexturesOfMaterials::GetImageFromImageData(const ImageData &image_data, vk::Format format)
{
    if (image_data.GetComponentsCount() == 1 && format==vk::Format::eR8Srgb ||
       image_data.GetComponentsCount() == 2 && format==vk::Format::eR8G8Srgb ||
       image_data.GetComponentsCount() == 4 && format==vk::Format::eR8G8B8A8Srgb) {
        return image_data.GetImage(true, false);
    } else if (image_data.GetComponentsCount() == 1 && format==vk::Format::eR8Unorm ||
              image_data.GetComponentsCount() == 2 && format==vk::Format::eR8G8Unorm ||
              image_data.GetComponentsCount() == 4 && format==vk::Format::eR8G8B8A8Unorm) {
        return image_data.GetImage(false, false);
    } else if (image_data.GetComponentsCount() == 1 && format==vk::Format::eR16Unorm ||
              image_data.GetComponentsCount() == 2 && format==vk::Format::eR16G16Unorm ||
              image_data.GetComponentsCount() == 4 && format==vk::Format::eR16G16B16A16Unorm)  {
        return image_data.GetImage(false, true);
    } else {assert(0); return {};}
}
