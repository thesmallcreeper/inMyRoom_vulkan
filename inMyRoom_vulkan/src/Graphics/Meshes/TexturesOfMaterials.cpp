#include "Graphics/Meshes/TexturesOfMaterials.h"

#include <cstring>
#include <iostream>
#include <utility>
#include <cassert>
#include <algorithm>

#include "stb_image.h"
#include "stb_image_write.h"

#include "DDS_Helpers.h"
#include "Compressonator.h"

#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/sampler_create_info.h"

TexturesOfMaterials::TexturesOfMaterials(bool use_mipmaps,
                                         MipmapsGenerator* in_mipmapsGenerator_ptr,
                                         Anvil::BaseDevice* const in_device_ptr)
    :mipmapsGenerator_ptr(in_mipmapsGenerator_ptr),
     device_ptr(in_device_ptr),
     useMipmaps(use_mipmaps),
     imagesSoFar(0),
     texturesSoFar(0)
{
    // Default sampler ( its index is 0 )

    Anvil::SamplerUniquePtr image_sampler_ptr;

    auto create_info_ptr = Anvil::SamplerCreateInfo::create(device_ptr,
                                                            Anvil::Filter::LINEAR,
                                                            Anvil::Filter::LINEAR,
                                                            Anvil::SamplerMipmapMode::LINEAR,
                                                            Anvil::SamplerAddressMode::REPEAT,
                                                            Anvil::SamplerAddressMode::REPEAT,
                                                            Anvil::SamplerAddressMode::REPEAT,
                                                            0.0f, /* in_lod_bias        */
                                                            16.0f, /* in_max_anisotropy  */
                                                            false, /* in_compare_enable  */
                                                            Anvil::CompareOp::NEVER, /* in_compare_enable  */
                                                            0.0f, /* in_min_lod         */
                                                            16.0f, /* in_max_lod         */
                                                            Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                            false); /* in_use_unnormalized_coordinates */

    image_sampler_ptr = Anvil::Sampler::create(std::move(create_info_ptr));

    samplers_uptrs.emplace_back(std::move(image_sampler_ptr));

}


TexturesOfMaterials::~TexturesOfMaterials()
{
    samplers_uptrs.clear();
    imagesViews_upts.clear();
    images_uptrs.clear();
}

void TexturesOfMaterials::AddTexturesOfModel(const tinygltf::Model& in_model, const std::string in_imagesFolder)
{
    for (const tinygltf::Image& this_image : in_model.images)
    {
        std::unique_ptr<uint8_t[]> local_original_image_buffer;

        fs::path path_to_mipmap_folder;

        if (!this_image.uri.empty())
        {
            path_to_mipmap_folder = in_imagesFolder + "//" + this_image.uri.substr(0, this_image.uri.find_last_of('.')) + "_mipmaps";
            fs::create_directory(path_to_mipmap_folder);
        }
        else // gotta support that
        {
            assert(0);
        }

        mipmapsGenerator_ptr->BindNewImage(this_image, in_imagesFolder);
        MipmapInfo original_info = mipmapsGenerator_ptr->GetOriginalNullptr();

        size_t mipmaps_levels_over_4x4;
        if (useMipmaps)
        {
            mipmaps_levels_over_4x4 = mipmapsGenerator_ptr->GetMipmaps_levels_over_4x4();
        }
        else
        {
            mipmaps_levels_over_4x4 = 1;
        }

        std::vector<Anvil::MipmapRawData> compressed_mipmaps_raw_data;
        std::vector<void*> garbage_collector;

        for (size_t this_mipmap_level = 0; this_mipmap_level < mipmaps_levels_over_4x4; this_mipmap_level++)
        {
            std::string this_mipmap_filename = path_to_mipmap_folder.string() + "//mipmap_" + std::to_string(this_mipmap_level) + ".DDS";
            {
                CMP_Texture this_compressed_mipmap; //gotta delete at some point

                if (!LoadDDSFile(this_mipmap_filename.c_str(), this_compressed_mipmap))
                {
                    std::cout << "---" << this_image.uri << " , creating mipmap level: " << this_mipmap_level 
                              << " ,width= " << (original_info.width >> this_mipmap_level)
                              << " ,height= " << (original_info.height >> this_mipmap_level) << "\n";

                    std::cout << "----Creating mipmap\n";

                    MipmapInfo this_mipmap;

                    if (this_mipmap_level == 0)
                        this_mipmap = mipmapsGenerator_ptr->GetOriginal();
                    else
                        this_mipmap = mipmapsGenerator_ptr->GetMipmap(this_mipmap_level);

                    std::cout << "----Compressing mipmap\n";

                    CMP_Texture srcTexture;

                    srcTexture.dwSize = sizeof(srcTexture);
                    srcTexture.dwWidth = this_mipmap.width;
                    srcTexture.dwHeight = this_mipmap.height;
                    srcTexture.dwPitch = this_mipmap.pitch;
                    srcTexture.format = vulkanFormatToCompressonatorFormat_map.find(this_mipmap.image_vulkan_format)->second;
                    srcTexture.pData = this_mipmap.data_uptr.get();
                    srcTexture.dwDataSize = static_cast<CMP_DWORD>(this_mipmap.size);

                    CMP_Texture dstTexture;

                    dstTexture.dwSize = sizeof(dstTexture);
                    dstTexture.dwWidth = this_mipmap.width;
                    dstTexture.dwHeight = this_mipmap.height;
                    dstTexture.dwPitch = 0;
                    dstTexture.format = vulkanFormatToCompressonatorFormat_map.find(image_preferred_compressed_format)->second;
                    dstTexture.dwDataSize = CMP_CalculateBufferSize(&dstTexture);
                    dstTexture.pData = (CMP_BYTE*)malloc(dstTexture.dwDataSize);

                    CMP_CompressOptions options = { 0 };
                    options.dwSize = sizeof(options);
                    options.fquality = 0.08f;
                    options.dwnumThreads = 8;

                    CMP_ERROR cmp_status;
                    cmp_status = CMP_ConvertTexture(&srcTexture, &dstTexture, &options, nullptr, NULL, NULL);

                    if (cmp_status != CMP_OK)
                        assert(0);

                    SaveDDSFile(this_mipmap_filename.c_str(), dstTexture);

                    this_compressed_mipmap = dstTexture;
                }

                garbage_collector.emplace_back(this_compressed_mipmap.pData);

                compressed_mipmaps_raw_data.emplace_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                                                        static_cast<uint32_t>(this_mipmap_level),
                                                                                                        this_compressed_mipmap.pData,
                                                                                                        this_compressed_mipmap.dwDataSize,
                                                                                                        this_compressed_mipmap.dwDataSize / this_compressed_mipmap.dwHeight));
            }
        }

        Anvil::ImageUniquePtr image_ptr;
        Anvil::ImageViewUniquePtr image_view_ptr;

        {
            auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(device_ptr,
                                                                        Anvil::ImageType::_2D,
                                                                        image_preferred_compressed_format,
                                                                        Anvil::ImageTiling::OPTIMAL,
                                                                        Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT,
                                                                        static_cast<uint32_t>(original_info.width),
                                                                        static_cast<uint32_t>(original_info.height),
                                                                        1, /* base_mipmap_depth */
                                                                        1, /* n_layers */
                                                                        Anvil::SampleCountFlagBits::_1_BIT,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        useMipmaps, /* in_use_full_mipmap_chain */
                                                                        Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT,
                                                                        Anvil::ImageCreateFlagBits::NONE,
                                                                        Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL, /* in_final_image_layout    */
                                                                        &compressed_mipmaps_raw_data);

            image_ptr = Anvil::Image::create(std::move(create_info_ptr));
        }

        for (void* this_garbage : garbage_collector)
            std::free(this_garbage);

        {
            auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(device_ptr,
                                                                         image_ptr.get(),
                                                                         0,                                                /* n_base_layer        */
                                                                         0,                                                /* n_base_mipmap_level */
                                                                         static_cast<uint32_t>(mipmaps_levels_over_4x4),   /* n_mipmaps           */
                                                                         Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                         image_preferred_compressed_format,
                                                                         Anvil::ComponentSwizzle::R,
                                                                         (original_info.image_vulkan_format == Anvil::Format::R8G8_SRGB ||
                                                                          original_info.image_vulkan_format == Anvil::Format::R8G8B8_SRGB ||
                                                                          original_info.image_vulkan_format == Anvil::Format::R8G8B8A8_SRGB)
                                                                             ? Anvil::ComponentSwizzle::G
                                                                             : Anvil::ComponentSwizzle::ZERO,
                                                                         (original_info.image_vulkan_format == Anvil::Format::R8G8B8_SRGB ||
                                                                          original_info.image_vulkan_format == Anvil::Format::R8G8B8A8_SRGB)
                                                                             ? Anvil::ComponentSwizzle::B
                                                                             : Anvil::ComponentSwizzle::ZERO,
                                                                         (original_info.image_vulkan_format == Anvil::Format::R8G8B8A8_SRGB)
                                                                             ? Anvil::ComponentSwizzle::A
                                                                             : Anvil::ComponentSwizzle::ONE);

            image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr));
        }

        images_uptrs.emplace_back(std::move(image_ptr));
        imagesViews_upts.emplace_back(std::move(image_view_ptr));
    }

    mipmapsGenerator_ptr->Reset();

    for (const tinygltf::Texture& this_texture : in_model.textures)
    {
        TextureInfo this_texture_info;

        this_texture_info.imageIndex = imagesSoFar + this_texture.source;

        if (this_texture.sampler != -1)
        {
            const tinygltf::Sampler& this_sampler = in_model.samplers[this_texture.sampler];

            SamplerSpecs this_sampler_specs;
            this_sampler_specs.magFilter = static_cast<glTFsamplerMagFilter>(this_sampler.magFilter);
            this_sampler_specs.minFilter = static_cast<glTFsamplerMinFilter>(this_sampler.minFilter);
            this_sampler_specs.wrapS = static_cast<glTFsamplerWrap>(this_sampler.wrapS);
            this_sampler_specs.wrapT = static_cast<glTFsamplerWrap>(this_sampler.wrapT);

            this_texture_info.samplerIndex = GetSamplerIndex(this_sampler_specs);
        }
        else
            this_texture_info.samplerIndex = 0;

        texturesInfos.emplace_back(this_texture_info);

    }

    imagesSoFar += in_model.images.size();
    
    modelToTextureIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), texturesSoFar);
    texturesSoFar += in_model.textures.size();
}

size_t TexturesOfMaterials::GetSamplerIndex(SamplerSpecs in_samplerSpecs)
{
    auto search = samplerSpecsToSamplerIndex_umap.find(in_samplerSpecs);
    if (search != samplerSpecsToSamplerIndex_umap.end())
    {
        return search->second;
    }
    else
    {
        Anvil::SamplerUniquePtr image_sampler_uptr;

        auto create_info_ptr = Anvil::SamplerCreateInfo::create(device_ptr,
                                                                glTFsamplerMagFilterToFilter_map.find(static_cast<glTFsamplerMagFilter>(in_samplerSpecs.magFilter))->second,
                                                                glTFsamplerMinFilterToFilterAndMipmapMode_map.find(static_cast<glTFsamplerMinFilter>(in_samplerSpecs.minFilter))->second.first,
                                                                glTFsamplerMinFilterToFilterAndMipmapMode_map.find(static_cast<glTFsamplerMinFilter>(in_samplerSpecs.minFilter))->second.second,
                                                                glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(in_samplerSpecs.wrapS))->second,
                                                                glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(in_samplerSpecs.wrapT))->second,
                                                                Anvil::SamplerAddressMode::REPEAT,
                                                                0.0f, /* in_lod_bias        */
                                                                16.0f, /* in_max_anisotropy  */
                                                                false, /* in_compare_enable  */
                                                                Anvil::CompareOp::NEVER,    /* in_compare_enable  */
                                                                0.0f, /* in_min_lod         */
                                                                16.0f, /* in_min_lod         */
                                                                Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                                false); /* in_use_unnormalized_coordinates */

        image_sampler_uptr = Anvil::Sampler::create(std::move(create_info_ptr));

        samplers_uptrs.emplace_back(std::move(image_sampler_uptr));

        size_t index_in_vector = samplers_uptrs.size() - 1;
        samplerSpecsToSamplerIndex_umap.emplace(in_samplerSpecs, index_in_vector);

        return index_in_vector;
    }
}

size_t TexturesOfMaterials::GetTextureIndexOffsetOfModel(const tinygltf::Model& in_model)
{
    auto search = modelToTextureIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToTextureIndexOffset_umap.end());

    return search->second;
}

Anvil::ImageView* TexturesOfMaterials::GetImageView(size_t in_texture_index)
{
    TextureInfo this_textureInfo = texturesInfos[in_texture_index];

    return  imagesViews_upts[this_textureInfo.imageIndex].get();
}

Anvil::Sampler* TexturesOfMaterials::GetSampler(size_t in_texture_index)
{
    TextureInfo this_textureInfo = texturesInfos[in_texture_index];

    return  samplers_uptrs[this_textureInfo.samplerIndex].get();
}