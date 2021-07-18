#include "Graphics/MipmapsGenerator.h"

#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/framebuffer_create_info.h"
#include "misc/sampler_create_info.h"
#include "misc/render_pass_create_info.h"
#include "misc/buffer_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/fence_create_info.h"

#include "Applications/_Plugins/Common/stb_image.h"

#include <cmath>

#include "glm/vec3.hpp"
#include "glm/vec2.hpp"

MipmapsGenerator::MipmapsGenerator(PipelinesFactory* in_pipelinesFactory_ptr,
                                   ShadersSetsFamiliesCache* in_shadersSetsFamiliesCache_ptr,
                                   ImagesAboutOfTextures* in_imagesAboutOfTextures_ptr,
                                   std::string in_16bitTo8bit_shadername,
                                   std::string in_baseColor_shadername,
                                   std::string in_occlusionMetallicRoughness_shadername,
                                   std::string in_normal_shadername,
                                   std::string in_emissive_shadername,
                                   Anvil::PrimaryCommandBuffer* const in_cmd_buffer_ptr,
                                   Anvil::BaseDevice* const in_device_ptr)
    :pipelinesFactory_ptr(in_pipelinesFactory_ptr),
     shadersSetsFamiliesCache_ptr(in_shadersSetsFamiliesCache_ptr),
     imagesAboutOfTextures_ptr(in_imagesAboutOfTextures_ptr),
     cmd_buffer_ptr(in_cmd_buffer_ptr),
     device_ptr(in_device_ptr)
{
    _16bitTo8bit_shadername = in_16bitTo8bit_shadername;

    baseColor_shadername = in_baseColor_shadername;
    occlusionMetallicRoughness_shadername = in_occlusionMetallicRoughness_shadername;
    normal_shadername = in_normal_shadername;
    emissive_shadername = in_emissive_shadername;

    isImageLoaded = false;

    // Init vertex bvuffers
    InitQuadIndexBuffer();
    InitQuadPositionBuffer();
    InitQuadTexcoordBuffer();
}

MipmapsGenerator::~MipmapsGenerator()
{
    Reset();
}

void MipmapsGenerator::Reset()
{
    alignedDefault_8bitPerChannel_image_uptr.reset();
    alignedDefault_8bitPerChannel_imageView_uptr.reset();

    shaderSetName = "";
    isImageLoaded = false;
}

void MipmapsGenerator::InitQuadIndexBuffer()
{
    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    std::vector<uint32_t> index = {
        // indexes        
        0, 1, 2,
        2, 1, 3
    };

    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    sizeof(uint32_t) * index.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    Anvil::BufferUsageFlagBits::INDEX_BUFFER_BIT);

    quadIndexBuffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

    quadIndexBuffer_uptr->set_name("QuadIndexBuffer");

    allocator_ptr->add_buffer(quadIndexBuffer_uptr.get(),
                                Anvil::MemoryFeatureFlagBits::NONE);

    quadIndexBuffer_uptr->write(0,
                                sizeof(uint32_t) * index.size(),
                                index.data());
}

void MipmapsGenerator::InitQuadPositionBuffer()
{
    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    std::vector<float> position = {
        // positions         
        -1.0f, -1.0f, 0.1f,
        -1.0f,  1.0f, 0.1f,
         1.0f, -1.0f, 0.1f,
         1.0f,  1.0f, 0.1f
    };

    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    sizeof(float) * position.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);

    quadPositionBuffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

    quadPositionBuffer_uptr->set_name("QuadPositionBuffer");

    allocator_ptr->add_buffer(quadPositionBuffer_uptr.get(),
                                Anvil::MemoryFeatureFlagBits::NONE);

    quadPositionBuffer_uptr->write(0,
                                    sizeof(float) * position.size(),
                                    position.data());
}

void MipmapsGenerator::InitQuadTexcoordBuffer()
{
    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    std::vector<float> texcoords = {
        // texcoords
        0.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };

    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    sizeof(float) * texcoords.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);

    quadTexcoordsBuffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

    quadTexcoordsBuffer_uptr->set_name("QuadTexcoordsBuffer");

    allocator_ptr->add_buffer(quadTexcoordsBuffer_uptr.get(),
                                Anvil::MemoryFeatureFlagBits::NONE);

    quadTexcoordsBuffer_uptr->write(0,
                                    sizeof(float) * texcoords.size(),
                                    texcoords.data());
}

void MipmapsGenerator::InitComputeSampler()
{
    auto create_info_ptr = Anvil::SamplerCreateInfo::create(device_ptr,
                                                            Anvil::Filter::NEAREST,
                                                            Anvil::Filter::NEAREST,
                                                            Anvil::SamplerMipmapMode::NEAREST,
                                                            glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(imageAbout.wrapS))->second,
                                                            glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(imageAbout.wrapT))->second,
                                                            Anvil::SamplerAddressMode::REPEAT,
                                                            0.0f, /* in_lod_bias        */
                                                            1.0f, /* in_max_anisotropy  */
                                                            false, /* in_compare_enable  */
                                                            Anvil::CompareOp::NEVER,    /* in_compare_enable  */
                                                            0.0f, /* in_min_lod         */
                                                            0.0f, /* in_min_lod         */
                                                            Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                            false); /* in_use_unnormalized_coordinates */

    imageComputeSampler_uptr = Anvil::Sampler::create(std::move(create_info_ptr));
}

void MipmapsGenerator::InitRenderpassSampler()
{
    auto create_info_ptr = Anvil::SamplerCreateInfo::create(device_ptr,
                                                            Anvil::Filter::NEAREST,
                                                            Anvil::Filter::NEAREST,
                                                            Anvil::SamplerMipmapMode::NEAREST,
                                                            glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(imageAbout.wrapS))->second,
                                                            glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(imageAbout.wrapT))->second,
                                                            Anvil::SamplerAddressMode::REPEAT,
                                                            0.0f, /* in_lod_bias        */
                                                            1.0f, /* in_max_anisotropy  */
                                                            false, /* in_compare_enable  */
                                                            Anvil::CompareOp::NEVER,    /* in_compare_enable  */
                                                            0.0f, /* in_min_lod         */
                                                            0.0f, /* in_min_lod         */
                                                            Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                            false); /* in_use_unnormalized_coordinates */

    imageRenderpassSampler_uptr = Anvil::Sampler::create(std::move(create_info_ptr));
}

void MipmapsGenerator::BindNewImage(const tinygltf::Image& image, const std::string images_folder)
{
    imagesFolder = images_folder;
    Reset();

    MipmapInfo new_image;

    imageAbout = imagesAboutOfTextures_ptr->GetImageAbout(image);

    if (imageAbout.map == ImageMap::metallicRoughness || imageAbout.map == (ImageMap::metallicRoughness | ImageMap::occlusion))
    {
        MipmapInfo metallicRoughness_mipmapinfo = LoadImageFileFromDisk(image, images_folder);

        MipmapInfo roughness_mipmapinfo;
        if (imageAbout.sibling_occlusion_image_ptr != nullptr)
        {
            roughness_mipmapinfo = LoadImageFileFromDisk(*imageAbout.sibling_occlusion_image_ptr, images_folder);
            assert(metallicRoughness_mipmapinfo.width == roughness_mipmapinfo.width && metallicRoughness_mipmapinfo.height == roughness_mipmapinfo.height);
        }
        else
        {
            roughness_mipmapinfo.width = metallicRoughness_mipmapinfo.width;
            roughness_mipmapinfo.height = metallicRoughness_mipmapinfo.height;
            roughness_mipmapinfo.pitch = metallicRoughness_mipmapinfo.pitch;
            roughness_mipmapinfo.defaultCompCount = 1;
            roughness_mipmapinfo.size = roughness_mipmapinfo.width * roughness_mipmapinfo.height * roughness_mipmapinfo.defaultCompCount;
            roughness_mipmapinfo.gammaCorrection = false;

            roughness_mipmapinfo.data_uptr.reset(new uint8_t[roughness_mipmapinfo.size]);
            std::memset(roughness_mipmapinfo.data_uptr.get(), 0, roughness_mipmapinfo.size);
        }

        shaderSetName = occlusionMetallicRoughness_shadername;
        new_image = MergeOcclusionWithMetallicRoughness(roughness_mipmapinfo, metallicRoughness_mipmapinfo);
    }
    else if (imageAbout.map == ImageMap::occlusion)
    {
        assert(imageAbout.sibling_metallicRoughness_image_ptr == nullptr);

        MipmapInfo roughness_mipmapinfo = LoadImageFileFromDisk(image, images_folder);

        MipmapInfo metallicRoughness_mipmapinfo;
        metallicRoughness_mipmapinfo.width = roughness_mipmapinfo.width;
        metallicRoughness_mipmapinfo.height = roughness_mipmapinfo.height;
        metallicRoughness_mipmapinfo.pitch = roughness_mipmapinfo.pitch;
        metallicRoughness_mipmapinfo.defaultCompCount = 3;
        metallicRoughness_mipmapinfo.size = metallicRoughness_mipmapinfo.width * metallicRoughness_mipmapinfo.height * metallicRoughness_mipmapinfo.defaultCompCount;
        metallicRoughness_mipmapinfo.gammaCorrection = false;

        metallicRoughness_mipmapinfo.data_uptr.reset(new uint8_t[metallicRoughness_mipmapinfo.size]);
        std::memset(metallicRoughness_mipmapinfo.data_uptr.get(), 0, metallicRoughness_mipmapinfo.size);

        shaderSetName = occlusionMetallicRoughness_shadername;
        new_image = MergeOcclusionWithMetallicRoughness(roughness_mipmapinfo, metallicRoughness_mipmapinfo);
    }
    else
    {
        shaderSetName = baseColor_shadername;
        new_image = LoadImageFileFromDisk(image, images_folder);
    }

    original_width = new_image.width;
    original_height = new_image.height;
    defaultImageCompCount = new_image.defaultCompCount;
    doesUseGamma = new_image.gammaCorrection;
    alignedImageCompCount = 4;

    localDefaultImage_buffer = std::move(new_image.data_uptr);
}

MipmapInfo MipmapsGenerator::LoadImageFileFromDisk(const tinygltf::Image& image, const std::string images_folder)
{
    MipmapInfo return_mipmapInfo;

    if (!image.uri.empty())
    {
        fs::path path_to_original_image = images_folder + "//" + image.uri;

        fs::path absolute_path_to_original_image = fs::absolute(path_to_original_image);

        unsigned char* stbi_data = stbi_load(absolute_path_to_original_image.generic_string().c_str(),
                                             (int*)&return_mipmapInfo.width,
                                             (int*)&return_mipmapInfo.height,
                                             (int*)&return_mipmapInfo.defaultCompCount,
                                             0);
        return_mipmapInfo.pitch = return_mipmapInfo.width * return_mipmapInfo.defaultCompCount;
        return_mipmapInfo.size = return_mipmapInfo.width * return_mipmapInfo.height * return_mipmapInfo.defaultCompCount;

        assert(stbi_data);

        return_mipmapInfo.data_uptr.reset(new uint8_t[return_mipmapInfo.size]);
        std::memcpy(return_mipmapInfo.data_uptr.get(), stbi_data, return_mipmapInfo.size);

        if (imagesAboutOfTextures_ptr->GetImageAbout(image).map != ImageMap::normal &&
            imagesAboutOfTextures_ptr->GetImageAbout(image).map != ImageMap::occlusion &&
            imagesAboutOfTextures_ptr->GetImageAbout(image).map != ImageMap::metallicRoughness &&
            imagesAboutOfTextures_ptr->GetImageAbout(image).map != (ImageMap::occlusion | ImageMap::metallicRoughness))
            return_mipmapInfo.gammaCorrection = true;
        else
            return_mipmapInfo.gammaCorrection = false;

        stbi_image_free(stbi_data);
    }
    else // gotta support that TODO
    {
        assert(0);
    }

    return std::move(return_mipmapInfo);
}

MipmapInfo MipmapsGenerator::MergeOcclusionWithMetallicRoughness(MipmapInfo& ref_occlusion_map, MipmapInfo& ref_metallicRoughness_map)
{
    assert(ref_occlusion_map.width == ref_metallicRoughness_map.width);
    assert(ref_occlusion_map.height == ref_metallicRoughness_map.height);
    assert(ref_occlusion_map.pitch == ref_metallicRoughness_map.pitch);

    MipmapInfo return_mipmapInfo;
    return_mipmapInfo.defaultCompCount = 3;
    return_mipmapInfo.width = ref_metallicRoughness_map.width;
    return_mipmapInfo.height = ref_metallicRoughness_map.height;
    return_mipmapInfo.pitch = return_mipmapInfo.defaultCompCount * return_mipmapInfo.width;
    return_mipmapInfo.size = return_mipmapInfo.height * return_mipmapInfo.width * return_mipmapInfo.defaultCompCount;
    return_mipmapInfo.data_uptr.reset(new uint8_t[return_mipmapInfo.size]);

    for (size_t i = 0; i < return_mipmapInfo.size; i++)
    {
        if (i % 3 == 0)
            return_mipmapInfo.data_uptr[i] = ref_occlusion_map.data_uptr[ref_occlusion_map.defaultCompCount * (i / 3)];                            // R occlusion
        else if (i % 3 == 1)
            return_mipmapInfo.data_uptr[i] = ref_metallicRoughness_map.data_uptr[ref_metallicRoughness_map.defaultCompCount * (i / 3) + 1];        // G roughness
        else if (i % 3 == 2)
            return_mipmapInfo.data_uptr[i] = ref_metallicRoughness_map.data_uptr[ref_metallicRoughness_map.defaultCompCount * (i / 3) + 2];        // B metallic
    }

    return std::move(return_mipmapInfo);
}

void MipmapsGenerator::LoadImageToGPU()
{
    // Align image
    std::unique_ptr<uint8_t[]> aligned_image_buffer;
    aligned_image_buffer = CopyToLocalBuffer(localDefaultImage_buffer.get(), defaultImageCompCount * original_width * original_height, (defaultImageCompCount != 3) ? false : true);

    alignedImageSize = alignedImageCompCount * original_width * original_height;
    if (doesUseGamma)
        vulkanAlignedFormat = componentsCountToVulkanGammaFormat_map.find(alignedImageCompCount)->second;
    else
        vulkanAlignedFormat = componentsCountToVulkanLinearFormat_map.find(alignedImageCompCount)->second;

    InitGPUimageAndView(aligned_image_buffer.get(), alignedImageSize);

    // Init samplers
    InitComputeSampler();
    InitRenderpassSampler();

    isImageLoaded = true;
}

void MipmapsGenerator::InitGPUimageAndView(uint8_t* in_image_raw, size_t image_size)
{
    {
        std::vector<Anvil::MipmapRawData> aligned_image_mipmapRawData;
        aligned_image_mipmapRawData.emplace_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                                                0,
                                                                                                in_image_raw,
                                                                                                static_cast<uint32_t>(image_size),
                                                                                                static_cast<uint32_t>(image_size / original_height)));

        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(device_ptr,
                                                                    Anvil::ImageType::_2D,
                                                                    vulkanAlignedFormat,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::SAMPLED_BIT,
                                                                    static_cast<uint32_t>(original_width),
                                                                    static_cast<uint32_t>(original_height),
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false, /* in_use_full_mipmap_chain */
                                                                    Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT,
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL, /* in_final_image_layout    */
                                                                    &aligned_image_mipmapRawData);

        alignedDefault_8bitPerChannel_image_uptr = Anvil::Image::create(std::move(create_info_ptr));
    }

    {
        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(device_ptr,
                                                                     alignedDefault_8bitPerChannel_image_uptr.get(),
                                                                     0, /* n_base_layer        */
                                                                     0, /* n_base_mipmap_level */
                                                                     1, /* n_mipmaps           */
                                                                     Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                     vulkanAlignedFormat,
                                                                     Anvil::ComponentSwizzle::R,
                                                                     (alignedImageCompCount >= 2)
                                                                     ? Anvil::ComponentSwizzle::G
                                                                     : Anvil::ComponentSwizzle::ZERO,
                                                                     (alignedImageCompCount >= 3)
                                                                     ? Anvil::ComponentSwizzle::B
                                                                     : Anvil::ComponentSwizzle::ZERO,
                                                                     (alignedImageCompCount >= 4)
                                                                     ? Anvil::ComponentSwizzle::A
                                                                     : Anvil::ComponentSwizzle::ONE);

        alignedDefault_8bitPerChannel_imageView_uptr = Anvil::ImageView::create(std::move(create_info_ptr));
    }
}


MipmapInfo MipmapsGenerator::GetMipmap(size_t mipmap_level)
{
    assert(mipmap_level > 0);

    if (!isImageLoaded)
        LoadImageToGPU();

    uint32_t this_mipmap_width = std::max<uint32_t>(original_width >> mipmap_level, 4);
    uint32_t this_mipmap_height = std::max<uint32_t>(original_height >> mipmap_level, 4);

    Anvil::ImageUniquePtr mipmap_16bitPerChannel_image_uptr;
    Anvil::ImageViewUniquePtr mipmap_16bitPerChannel_imageView_uptr;

    Anvil::Format vulkan_aligned_16BitPerChannelFormat = vulkanFormatTo16BitPerChannel_map.find(vulkanAlignedFormat)->second;
    Anvil::ImageUniquePtr mipmap_8bitPerChannel_image_uptr;
    Anvil::ImageViewUniquePtr mipmap_8bitPerChannel_imageView_uptr;

    Anvil::DescriptorSetGroupUniquePtr descriptorSetGroup_uptr;

    Anvil::BufferUniquePtr device_to_host_buffer;

    Anvil::PipelineID this_compute_pipelineID;
    Anvil::PipelineID this_gfx_pipelineID;

    Anvil::FramebufferUniquePtr framebuffer_uptr;

    // 16-bit compute-output channel format
    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(device_ptr,
                                                                    Anvil::ImageType::_2D,
                                                                    vulkan_aligned_16BitPerChannelFormat,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::STORAGE_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT,
                                                                    static_cast<uint32_t>(this_mipmap_width),
                                                                    static_cast<uint32_t>(this_mipmap_height),
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::COMPUTE_BIT | Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false, /* in_use_full_mipmap_chain */
                                                                    Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT,
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::GENERAL, /* in_final_image_layout    */
                                                                    nullptr);

        mipmap_16bitPerChannel_image_uptr = Anvil::Image::create(std::move(create_info_ptr));
    }

    {
        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(device_ptr,
                                                                     mipmap_16bitPerChannel_image_uptr.get(),
                                                                     0, /* n_base_layer        */
                                                                     0, /* n_base_mipmap_level */
                                                                     1, /* n_mipmaps           */
                                                                     Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                     vulkan_aligned_16BitPerChannelFormat,
                                                                     Anvil::ComponentSwizzle::R,
                                                                     (alignedImageCompCount >= 2)
                                                                     ? Anvil::ComponentSwizzle::G
                                                                     : Anvil::ComponentSwizzle::ZERO,
                                                                     (alignedImageCompCount >= 3)
                                                                     ? Anvil::ComponentSwizzle::B
                                                                     : Anvil::ComponentSwizzle::ZERO,
                                                                     (alignedImageCompCount >= 4)
                                                                     ? Anvil::ComponentSwizzle::A
                                                                     : Anvil::ComponentSwizzle::ONE);

        mipmap_16bitPerChannel_imageView_uptr = Anvil::ImageView::create(std::move(create_info_ptr));
    }

    {
        // Create description set for original 8-bit images and storage of 16-bit mipmap
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dsg_create_infos_ptr;
        {
            Anvil::DescriptorSetCreateInfoUniquePtr this_descriptorSetCreateInfo_ptr = Anvil::DescriptorSetCreateInfo::create();

            this_descriptorSetCreateInfo_ptr->add_binding(0,
                                                          Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                          1,
                                                          Anvil::ShaderStageFlagBits::COMPUTE_BIT);

            dsg_create_infos_ptr.emplace_back(std::move(this_descriptorSetCreateInfo_ptr));
        }
        {
            Anvil::DescriptorSetCreateInfoUniquePtr this_descriptorSetCreateInfo_ptr = Anvil::DescriptorSetCreateInfo::create();

            this_descriptorSetCreateInfo_ptr->add_binding(0,
                                                          Anvil::DescriptorType::STORAGE_IMAGE,
                                                          1,
                                                          Anvil::ShaderStageFlagBits::COMPUTE_BIT);

            dsg_create_infos_ptr.emplace_back(std::move(this_descriptorSetCreateInfo_ptr));
        }

        // Create descriptor set for 16-bit to 8-bit
        {
            Anvil::DescriptorSetCreateInfoUniquePtr this_descriptorSetCreateInfo_ptr = Anvil::DescriptorSetCreateInfo::create();

            this_descriptorSetCreateInfo_ptr->add_binding(0,
                                                          Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                          1,
                                                          Anvil::ShaderStageFlagBits::FRAGMENT_BIT);

            dsg_create_infos_ptr.emplace_back(std::move(this_descriptorSetCreateInfo_ptr));
        }

        descriptorSetGroup_uptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                                    dsg_create_infos_ptr,
                                                                    false);           /* in_releaseable_sets */
    }

    // Update DS 0/0 : sampled 8bit image (source)
    {
        Anvil::DescriptorSet::CombinedImageSamplerBindingElement this_texture_bind(Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                                                   alignedDefault_8bitPerChannel_imageView_uptr.get(),
                                                                                   imageComputeSampler_uptr.get());

        descriptorSetGroup_uptr->set_binding_item(0, 0, this_texture_bind);
    }
    // Update DS 1/0 : storage 16bit image (destination)
    {
        Anvil::DescriptorSet::StorageImageBindingElement this_image_bind(Anvil::ImageLayout::GENERAL,
                                                                         mipmap_16bitPerChannel_imageView_uptr.get());



        descriptorSetGroup_uptr->set_binding_item(1, 0, this_image_bind);
    }
    // Update DS 2/0 : sampled 16bit image (source)
    {
        Anvil::DescriptorSet::CombinedImageSamplerBindingElement this_texture_bind(Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                                                   mipmap_16bitPerChannel_imageView_uptr.get(),
                                                                                   imageRenderpassSampler_uptr.get());

        descriptorSetGroup_uptr->set_binding_item(2, 0, this_texture_bind);
    }


    // 8-bit output channel format
    {
        auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(device_ptr,
                                                                    Anvil::ImageType::_2D,
                                                                    vulkanAlignedFormat,
                                                                    Anvil::ImageTiling::OPTIMAL,
                                                                    Anvil::ImageUsageFlagBits::COLOR_ATTACHMENT_BIT | Anvil::ImageUsageFlagBits::TRANSFER_SRC_BIT,
                                                                    static_cast<uint32_t>(this_mipmap_width),
                                                                    static_cast<uint32_t>(this_mipmap_height),
                                                                    1, /* base_mipmap_depth */
                                                                    1, /* n_layers */
                                                                    Anvil::SampleCountFlagBits::_1_BIT,
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    false, /* in_use_full_mipmap_chain */
                                                                    Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT,
                                                                    Anvil::ImageCreateFlagBits::NONE,
                                                                    Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL, /* in_final_image_layout    */
                                                                    nullptr);

        mipmap_8bitPerChannel_image_uptr = Anvil::Image::create(std::move(create_info_ptr));
    }

    {
        auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(device_ptr,
                                                                     mipmap_8bitPerChannel_image_uptr.get(),
                                                                     0, /* n_base_layer        */
                                                                     0, /* n_base_mipmap_level */
                                                                     1, /* n_mipmaps           */
                                                                     Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                     vulkanAlignedFormat,
                                                                     Anvil::ComponentSwizzle::R,
                                                                     (alignedImageCompCount >= 2)
                                                                     ? Anvil::ComponentSwizzle::G
                                                                     : Anvil::ComponentSwizzle::ZERO,
                                                                     (alignedImageCompCount >= 3)
                                                                     ? Anvil::ComponentSwizzle::B
                                                                     : Anvil::ComponentSwizzle::ZERO,
                                                                     (alignedImageCompCount >= 4)
                                                                     ? Anvil::ComponentSwizzle::A
                                                                     : Anvil::ComponentSwizzle::ONE);

        mipmap_8bitPerChannel_imageView_uptr = Anvil::ImageView::create(std::move(create_info_ptr));
    }

    {
        auto create_info_ptr = Anvil::BufferCreateInfo::create_alloc(device_ptr,
                                                                     this_mipmap_width * this_mipmap_height * alignedImageCompCount,
                                                                     Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                     Anvil::SharingMode::EXCLUSIVE,
                                                                     Anvil::BufferCreateFlagBits::NONE,
                                                                     Anvil::BufferUsageFlagBits::TRANSFER_DST_BIT,
                                                                     Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT | Anvil::MemoryFeatureFlagBits::MAPPABLE_BIT | Anvil::MemoryFeatureFlagBits::HOST_COHERENT_BIT);

        device_to_host_buffer = Anvil::Buffer::create(std::move(create_info_ptr));
    }


    // Create compute pipeline for filtering
    {
        ShadersSpecs this_shader_specs;
        this_shader_specs.shadersSetFamilyName = shaderSetName;
        if (alignedImageCompCount == 1)
            this_shader_specs.emptyDefinition.emplace_back("IS_R");
        else if (alignedImageCompCount == 2)
            this_shader_specs.emptyDefinition.emplace_back("IS_RG");
        else if(alignedImageCompCount == 4)
            this_shader_specs.emptyDefinition.emplace_back("IS_RGBA");

        ShadersSet this_shader_set = shadersSetsFamiliesCache_ptr->GetShadersSet(this_shader_specs);

        ComputePipelineSpecs this_compute_pipeline_specs;
        this_compute_pipeline_specs.pipelineShaders = this_shader_set;
         std::vector<const Anvil::DescriptorSetCreateInfo*> descriptor_sets_infos;
         descriptor_sets_infos.emplace_back(descriptorSetGroup_uptr->get_descriptor_set_create_info(0));
         descriptor_sets_infos.emplace_back(descriptorSetGroup_uptr->get_descriptor_set_create_info(1));
        this_compute_pipeline_specs.descriptorSetsCreateInfo_ptrs = descriptor_sets_infos;

        this_compute_pipelineID = pipelinesFactory_ptr->GetComputePipelineID(this_compute_pipeline_specs);
    }
    
    // Create renderpass to 16bit to 8bit
    Anvil::RenderPassUniquePtr    renderpass_uptr;
    Anvil::SubPassID              subpass16bitTo8bitID;
    Anvil::RenderPassCreateInfoUniquePtr renderpass_create_info_uptr(new Anvil::RenderPassCreateInfo(device_ptr));
    {
        Anvil::RenderPassAttachmentID color_attachment_id;
        renderpass_create_info_uptr->add_subpass(&subpass16bitTo8bitID);


        renderpass_create_info_uptr->add_external_to_subpass_dependency(subpass16bitTo8bitID,
                                                                        Anvil::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                                        Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT | Anvil::PipelineStageFlagBits::ALL_GRAPHICS_BIT,
                                                                        Anvil::AccessFlagBits::SHADER_WRITE_BIT | Anvil::AccessFlagBits::MEMORY_WRITE_BIT,
                                                                        Anvil::AccessFlagBits::SHADER_READ_BIT | Anvil::AccessFlagBits::MEMORY_READ_BIT,
                                                                        Anvil::DependencyFlagBits::BY_REGION_BIT);

        renderpass_create_info_uptr->add_color_attachment(vulkanAlignedFormat,
                                                         Anvil::SampleCountFlagBits::_1_BIT,
                                                         Anvil::AttachmentLoadOp::CLEAR,
                                                         Anvil::AttachmentStoreOp::STORE,
                                                         Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                         Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
                                                         false, /* may_alias */
                                                         &color_attachment_id);

        renderpass_create_info_uptr->add_subpass_color_attachment(subpass16bitTo8bitID,
                                                                  Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,
                                                                  color_attachment_id,
                                                                  0,        /* in_location                      */
                                                                  nullptr); /* in_opt_attachment_resolve_id_ptr */

        renderpass_create_info_uptr->add_subpass_to_external_dependency(subpass16bitTo8bitID,
                                                                        Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
                                                                        Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                                        Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,
                                                                        Anvil::AccessFlagBits::TRANSFER_READ_BIT | Anvil::AccessFlagBits::MEMORY_READ_BIT,
                                                                        Anvil::DependencyFlagBits::BY_REGION_BIT);

        renderpass_uptr = Anvil::RenderPass::create(std::move(renderpass_create_info_uptr), nullptr);
    }

    // Create graphics pipeline for 16bit to 8bit RGBA
    {
        ShadersSpecs this_shader_specs;
        this_shader_specs.shadersSetFamilyName = _16bitTo8bit_shadername;
        if (alignedImageCompCount == 1)
            this_shader_specs.emptyDefinition.emplace_back("IS_R");
        else if (alignedImageCompCount == 2)
            this_shader_specs.emptyDefinition.emplace_back("IS_RG");
        else if (alignedImageCompCount == 4)
            this_shader_specs.emptyDefinition.emplace_back("IS_RGBA");

        ShadersSet this_shader_set = shadersSetsFamiliesCache_ptr->GetShadersSet(this_shader_specs);

        GraphicsPipelineSpecs this_gfx_pipeline_specs;
        this_gfx_pipeline_specs.depthWriteEnable = false;
        this_gfx_pipeline_specs.depthCompare = Anvil::CompareOp::NEVER;
        this_gfx_pipeline_specs.drawMode = glTFmode::triangles;
        this_gfx_pipeline_specs.indexComponentType = glTFcomponentType::type_unsigned_int;
        this_gfx_pipeline_specs.positionComponentType = glTFcomponentType::type_float;
        this_gfx_pipeline_specs.texcoord0ComponentType = glTFcomponentType::type_float;
        this_gfx_pipeline_specs.renderpass_ptr = renderpass_uptr.get();
        this_gfx_pipeline_specs.subpassID = subpass16bitTo8bitID;
         std::vector<const Anvil::DescriptorSetCreateInfo*> this_gfx_descriptor_set;
         this_gfx_descriptor_set.emplace_back(descriptorSetGroup_uptr->get_descriptor_set_create_info(2));
        this_gfx_pipeline_specs.descriptorSetsCreateInfo_ptrs = this_gfx_descriptor_set;
        this_gfx_pipeline_specs.pipelineShaders = this_shader_set;
        this_gfx_pipeline_specs.viewportAndScissorSpecs.width = this_mipmap_width;
        this_gfx_pipeline_specs.viewportAndScissorSpecs.height = this_mipmap_height;

        this_gfx_pipelineID = pipelinesFactory_ptr->GetGraphicsPipelineID(this_gfx_pipeline_specs);
    }

    // Create framebuffer
    {
        auto create_info_ptr = Anvil::FramebufferCreateInfo::create(device_ptr,
                                                                    this_mipmap_width,
                                                                    this_mipmap_height,
                                                                    1); /* n_layers */

        create_info_ptr->add_attachment(mipmap_8bitPerChannel_imageView_uptr.get(),
                                        nullptr);

        framebuffer_uptr = Anvil::Framebuffer::create(std::move(create_info_ptr));
        framebuffer_uptr->set_name("8 bit per channel framebuffer");
    }

    // Start recording
    Anvil::Queue* queue_ptr = device_ptr->get_universal_queue(0);
    const uint32_t  universal_queue_family_index = device_ptr->get_universal_queue(0)->get_queue_family_index();
    {
        cmd_buffer_ptr->reset(true);

        cmd_buffer_ptr->start_recording(true,  /* one_time_submit          */
                                        false); /* simultaneous_use_allowed */

       // Call compute queue
        cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::COMPUTE,
                                             this_compute_pipelineID);
        {
            std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs;
            descriptor_sets_ptrs.emplace_back(descriptorSetGroup_uptr->get_descriptor_set(0));
            descriptor_sets_ptrs.emplace_back(descriptorSetGroup_uptr->get_descriptor_set(1));
            cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::COMPUTE,
                                                        device_ptr->get_compute_pipeline_manager()->get_pipeline_layout(this_compute_pipelineID),
                                                        0,
                                                        static_cast<uint32_t>(descriptor_sets_ptrs.size()),
                                                        descriptor_sets_ptrs.data(),
                                                        0,
                                                        nullptr);
        }

        cmd_buffer_ptr->record_dispatch(this_mipmap_width / 4, this_mipmap_height / 4, 1);

        {
            Anvil::ImageSubresourceRange  image_subresource_range;
            image_subresource_range.aspect_mask = Anvil::ImageAspectFlagBits::COLOR_BIT;
            image_subresource_range.base_array_layer = 0;
            image_subresource_range.base_mip_level = 0;
            image_subresource_range.layer_count = 1;
            image_subresource_range.level_count = 1;

            Anvil::ImageBarrier image_barrier(Anvil::AccessFlagBits::MEMORY_WRITE_BIT | Anvil::AccessFlagBits::SHADER_WRITE_BIT,   /* source_access_mask       */
                                              Anvil::AccessFlagBits::MEMORY_READ_BIT | Anvil::AccessFlagBits::SHADER_READ_BIT,                   /* destination_access_mask  */
                                              Anvil::ImageLayout::GENERAL,                              /* old_image_layout */
                                              Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,             /* new_image_layout */
                                              universal_queue_family_index,
                                              universal_queue_family_index,
                                              mipmap_16bitPerChannel_image_uptr.get(),
                                              image_subresource_range);

            cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::COMPUTE_SHADER_BIT,
                                                    Anvil::PipelineStageFlagBits::FRAGMENT_SHADER_BIT,
                                                    Anvil::DependencyFlagBits::BY_REGION_BIT,
                                                    0,
                                                    nullptr,
                                                    0,
                                                    nullptr,
                                                    1,
                                                    &image_barrier);

        }

        // Call gfx pipeline
        cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                             this_gfx_pipelineID);

        {
            // Start renderpass
            {
                VkClearValue  clear_values[1];
                clear_values[0].color.float32[0] = 0.0f;
                clear_values[0].color.float32[1] = 0.0f;
                clear_values[0].color.float32[2] = 0.0f;
                clear_values[0].color.float32[3] = 0.0f;

                VkRect2D  render_area;
                render_area.extent.width = this_mipmap_width;
                render_area.extent.height = this_mipmap_height;
                render_area.offset.x = 0;
                render_area.offset.y = 0;

                cmd_buffer_ptr->record_begin_render_pass(sizeof(clear_values) / sizeof(clear_values[0]),
                                                         clear_values,
                                                         framebuffer_uptr.get(),
                                                         render_area,
                                                         renderpass_uptr.get(),
                                                         Anvil::SubpassContents::INLINE);
            }

            {
                std::vector<Anvil::Buffer*> vertex_buffers;
                std::vector<VkDeviceSize> vertex_buffer_offsets;
                vertex_buffers.emplace_back(quadPositionBuffer_uptr.get());
                vertex_buffer_offsets.emplace_back(0);
                vertex_buffers.emplace_back(quadTexcoordsBuffer_uptr.get());
                vertex_buffer_offsets.emplace_back(0);

                cmd_buffer_ptr->record_bind_vertex_buffers(0,
                                                           static_cast<uint32_t>(vertex_buffers.size()),
                                                           vertex_buffers.data(),
                                                           vertex_buffer_offsets.data());
            }

            {
                std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs;
                descriptor_sets_ptrs.emplace_back(descriptorSetGroup_uptr->get_descriptor_set(2));
                cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                            device_ptr->get_graphics_pipeline_manager()->get_pipeline_layout(this_gfx_pipelineID),
                                                            0,
                                                            static_cast<uint32_t>(descriptor_sets_ptrs.size()),
                                                            descriptor_sets_ptrs.data(),
                                                            0,
                                                            nullptr);
            }

            cmd_buffer_ptr->record_bind_index_buffer(quadIndexBuffer_uptr.get(),
                                                     0,
                                                     Anvil::IndexType::UINT32);

            cmd_buffer_ptr->record_draw_indexed(6, 1, 0, 0, 0);

            cmd_buffer_ptr->record_end_render_pass();
        }

        {
             Anvil::ImageSubresourceRange  image_subresource_range;
             image_subresource_range.aspect_mask = Anvil::ImageAspectFlagBits::COLOR_BIT;
             image_subresource_range.base_array_layer = 0;
             image_subresource_range.base_mip_level = 0;
             image_subresource_range.layer_count = 1;
             image_subresource_range.level_count = 1;

             Anvil::ImageBarrier image_barrier(Anvil::AccessFlagBits::COLOR_ATTACHMENT_WRITE_BIT,                  /* source_access_mask       */
                                               Anvil::AccessFlagBits::TRANSFER_READ_BIT,                           /* destination_access_mask  */
                                               Anvil::ImageLayout::COLOR_ATTACHMENT_OPTIMAL,            /* old_image_layout */
                                               Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,               /* new_image_layout */
                                               universal_queue_family_index,
                                               universal_queue_family_index,
                                               mipmap_8bitPerChannel_image_uptr.get(),
                                               image_subresource_range);

//             cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::COLOR_ATTACHMENT_OUTPUT_BIT,
//                                                     Anvil::PipelineStageFlagBits::TRANSFER_BIT,
//                                                     Anvil::DependencyFlagBits::BY_REGION_BIT,
//                                                     0,
//                                                     nullptr,
//                                                     0,
//                                                     nullptr,
//                                                     1,
//                                                     &image_barrier);

        }

        {
            Anvil::BufferImageCopy this_copy;
            this_copy.buffer_offset = 0;
            this_copy.buffer_row_length = this_mipmap_width;
            this_copy.image_offset.x = 0;
            this_copy.image_offset.y = 0;
            this_copy.image_offset.z = 0;
            this_copy.image_extent.depth = 1;
            this_copy.image_extent.width = this_mipmap_width;
            this_copy.image_extent.height = this_mipmap_height;
            this_copy.image_subresource.mip_level = 0;
            this_copy.image_subresource.layer_count = 1;
            this_copy.image_subresource.base_array_layer = 0;
            this_copy.image_subresource.aspect_mask = Anvil::ImageAspectFlagBits::COLOR_BIT;

            cmd_buffer_ptr->record_copy_image_to_buffer(mipmap_8bitPerChannel_image_uptr.get(),
                                                        Anvil::ImageLayout::TRANSFER_SRC_OPTIMAL,
                                                        device_to_host_buffer.get(),
                                                        1,
                                                        &this_copy);
        }

        {
            Anvil::BufferBarrier buffer_barrier(Anvil::AccessFlagBits::TRANSFER_WRITE_BIT,
                                                Anvil::AccessFlagBits::HOST_READ_BIT,
                                                universal_queue_family_index,
                                                universal_queue_family_index,
                                                device_to_host_buffer.get(),
                                                0,
                                                device_to_host_buffer->get_create_info_ptr()->get_size());

            cmd_buffer_ptr->record_pipeline_barrier(Anvil::PipelineStageFlagBits::TRANSFER_BIT,
                                                    Anvil::PipelineStageFlagBits::HOST_BIT,
                                                    Anvil::DependencyFlagBits::BY_REGION_BIT,
                                                    0,
                                                    nullptr,
                                                    1,
                                                    &buffer_barrier,
                                                    0,
                                                    nullptr);
        }

        cmd_buffer_ptr->stop_recording();

    }


    {
        Anvil::PipelineStageFlags wait_stage_mask = Anvil::PipelineStageFlagBits::ALL_COMMANDS_BIT;

        queue_ptr->submit(
            Anvil::SubmitInfo::create(cmd_buffer_ptr,
                                        0,
                                        nullptr,
                                        0,
                                        nullptr,
                                        &wait_stage_mask,
                                        true /* should_block */,
                                        nullptr));
    }

    device_ptr->wait_idle();

    MipmapInfo this_mipmap_info;
    this_mipmap_info.data_uptr = CopyDeviceImageToLocalBuffer(device_to_host_buffer.get(), this_mipmap_width * this_mipmap_height * alignedImageCompCount, false);
    this_mipmap_info.width = this_mipmap_width;
    this_mipmap_info.height = this_mipmap_height;
    this_mipmap_info.pitch = this_mipmap_width * alignedImageCompCount;
    this_mipmap_info.size = this_mipmap_width * this_mipmap_height * alignedImageCompCount;
    this_mipmap_info.defaultCompCount = defaultImageCompCount;
    this_mipmap_info.gammaCorrection = doesUseGamma;
    this_mipmap_info.aligned_image_vulkan_format = vulkanAlignedFormat;

    return std::move(this_mipmap_info);
}


MipmapInfo MipmapsGenerator::GetAlignedOriginal()
{
    MipmapInfo return_mipmapInfo;
    return_mipmapInfo.data_uptr = CopyToLocalBuffer(localDefaultImage_buffer.get(),
                                                    original_width * original_height * defaultImageCompCount,
                                                    (defaultImageCompCount != 3) ? false : true);
    return_mipmapInfo.width = original_width;
    return_mipmapInfo.height = original_height;
    return_mipmapInfo.pitch = original_width * alignedImageCompCount;
    return_mipmapInfo.size = original_width * original_height * alignedImageCompCount;
    return_mipmapInfo.defaultCompCount = defaultImageCompCount;
    if (doesUseGamma)
        return_mipmapInfo.aligned_image_vulkan_format = componentsCountToVulkanGammaFormat_map.find(alignedImageCompCount)->second;
    else
        return_mipmapInfo.aligned_image_vulkan_format = componentsCountToVulkanLinearFormat_map.find(alignedImageCompCount)->second;

    return std::move(return_mipmapInfo);
}

MipmapInfo MipmapsGenerator::GetUnalignedInfo()
{
    MipmapInfo return_mipmapNullInfo;
    return_mipmapNullInfo.width = original_width;
    return_mipmapNullInfo.height = original_height;
    return_mipmapNullInfo.pitch = original_width * defaultImageCompCount;
    return_mipmapNullInfo.size = original_width * original_height * defaultImageCompCount;
    return_mipmapNullInfo.defaultCompCount = defaultImageCompCount;
    if (doesUseGamma)
        return_mipmapNullInfo.aligned_image_vulkan_format = componentsCountToVulkanGammaFormat_map.find(alignedImageCompCount)->second;
    else
        return_mipmapNullInfo.aligned_image_vulkan_format = componentsCountToVulkanLinearFormat_map.find(alignedImageCompCount)->second;

    return std::move(return_mipmapNullInfo);
}


std::unique_ptr<uint8_t[]> MipmapsGenerator::CopyDeviceImageToLocalBuffer(Anvil::Buffer* in_buffer, size_t image_size, bool shouldRGBAtoRGB)
{
    std::unique_ptr<uint8_t[]> mipmap_data_buffer;
    mipmap_data_buffer.reset(new uint8_t[image_size]);
    in_buffer->read(0, image_size, mipmap_data_buffer.get());

    std::unique_ptr<uint8_t[]> return_buffer;
    if (!shouldRGBAtoRGB)
    {
        return_buffer = std::move(mipmap_data_buffer);
    }
    else
    {
        return_buffer.reset(new uint8_t[(image_size / 4) * 3]);

        size_t output_index = 0;
        for (size_t input_index = 0; input_index < image_size; input_index++)
        {
            if (input_index % 4 != 3)
            {
                return_buffer[output_index] = mipmap_data_buffer[input_index];
                output_index++;
            }
        }
    }

    return std::move(return_buffer);
}

std::unique_ptr<uint8_t[]> MipmapsGenerator::CopyToLocalBuffer(uint8_t* in_buffer, size_t buffer_size, bool shouldRGBtoRGBA)
{
    std::unique_ptr<uint8_t[]> return_buffer;

    if (!shouldRGBtoRGBA)
    {
        return_buffer.reset(new uint8_t[buffer_size]);
        std::memcpy(return_buffer.get(), in_buffer, buffer_size);
    }
    else
    {
        return_buffer.reset(new uint8_t[(buffer_size / 3) * 4]);

        size_t input_index = 0;
        for (size_t output_index = 0; output_index < (buffer_size / 3) * 4; output_index++)
        {
            if (output_index % 4 != 3)
            {
                return_buffer[output_index] = in_buffer[input_index];
                input_index++;
            }
            else
                return_buffer[output_index] = (uint8_t)0xFFFF;
        }
    }

    return std::move(return_buffer);
}

size_t MipmapsGenerator::GetMipmaps_levels_over_4x4()
{
    size_t mipmaps_levels_over_4x4;

    if (original_width >= original_height)
        mipmaps_levels_over_4x4 = static_cast<size_t>(std::round(std::log2(original_width))) - 1;
    else
        mipmaps_levels_over_4x4 = static_cast<size_t>(std::round(std::log2(original_height))) - 1;

    return mipmaps_levels_over_4x4;
}
