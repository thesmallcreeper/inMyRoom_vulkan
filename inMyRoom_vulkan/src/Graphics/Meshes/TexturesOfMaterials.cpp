#include "Graphics/Meshes/TexturesOfMaterials.h"

#include <cstring>
#include <iostream>
#include <utility>
#include <cassert>
#include <algorithm>

#include "stb_image.h"
#include "stb_image_write.h"

#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/sampler_create_info.h"

TexturesOfMaterials::TexturesOfMaterials(bool use_mipmaps,
                                         ImagesAboutOfTextures* in_imagesAboutOfTextures_ptr,
                                         MipmapsGenerator* in_mipmapsGenerator_ptr,
                                         Anvil::BaseDevice* const in_device_ptr)
    :imageAboutOfTextures_ptr(in_imagesAboutOfTextures_ptr),
     mipmapsGenerator_ptr(in_mipmapsGenerator_ptr),
     device_ptr(in_device_ptr),
     useMipmaps(use_mipmaps)
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
    size_t thisModel_imagesFlashedSoFar = 0;
    size_t thisModel_imagesglTFcountSoFar = 0;
    std::unordered_map<size_t, size_t> imagesByglTFindex_to_imagesByFlashedIndex_umap;

    for (const tinygltf::Image& this_image : in_model.images)
    {
        ImageAbout this_image_about = imageAboutOfTextures_ptr->GetImageAbout(this_image);
        // because occlusion map will be baked with roughnessMetallic we ignore its texture
        if ( this_image_about.map != ImageMap::occlusion ||
            (this_image_about.map == ImageMap::occlusion && this_image_about.sibling_metallicRoughness_image_ptr == nullptr))
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
            MipmapInfo original_info = mipmapsGenerator_ptr->GetUnalignedInfo();

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
                    CMP_MipSet this_compressed_mipmap; // it gotta delete at some point
                    memset(&this_compressed_mipmap, 0, sizeof(CMP_MipSet));

                    if (CMP_LoadTexture(this_mipmap_filename.c_str(), &this_compressed_mipmap) != CMP_OK)
                    {
                        std::cout << "---" << this_image.uri << " , creating mipmap level: " << this_mipmap_level
                            << " ,width= " << (original_info.width >> this_mipmap_level)
                            << " ,height= " << (original_info.height >> this_mipmap_level) << "\n";

                        std::cout << "----Creating mipmap\n";

                        MipmapInfo this_mipmap;

                        if (this_mipmap_level == 0)
                            this_mipmap = mipmapsGenerator_ptr->GetAlignedOriginal();
                        else
                            this_mipmap = mipmapsGenerator_ptr->GetMipmap(this_mipmap_level);

                        std::cout << "----Compressing mipmap\n";

                        KernelOptions   kernel_options;
                        memset(&kernel_options, 0, sizeof(KernelOptions));

                        kernel_options.encodeWith = CMP_HPC;
                         Anvil::Format texture_target_compressed_format = vulkanFormatToCompressedFormat_map.find(this_mipmap.aligned_image_vulkan_format)->second;
                        kernel_options.format = vulkanFormatToCompressonatorFormat_map.find(texture_target_compressed_format)->second;
                        kernel_options.fquality = 0.1f;
                        kernel_options.threads = 0;

                        CMP_MipSet srcTexture;
                        CMP_MipLevel srcMipLevel;
                        memset(&srcTexture, 0, sizeof(CMP_MipSet));
                        memset(&srcMipLevel, 0, sizeof(CMP_MipLevel));

                        CMP_MipSet dstTexture;
                        memset(&dstTexture, 0, sizeof(CMP_MipSet));

                        srcTexture.m_nWidth = this_mipmap.width;
                        srcTexture.m_nHeight = this_mipmap.height;
                        srcTexture.m_nDepth = 1;
                        srcTexture.m_format = vulkanFormatToCompressonatorFormat_map.find(this_mipmap.aligned_image_vulkan_format)->second;
                        srcTexture.m_ChannelFormat = CF_8bit;
                        srcTexture.m_TextureDataType = this_mipmap.defaultCompCount == 3 ? TDT_XRGB : TDT_ARGB;
                        srcTexture.m_TextureType = TT_2D;
                        srcTexture.m_nMipLevels = 1;

                         CMP_MipLevelTable srcMipLevel_ptr = &srcMipLevel;
                        srcTexture.m_pMipLevelTable = &srcMipLevel_ptr;
                        srcMipLevel.m_nWidth = this_mipmap.width; 
                        srcMipLevel.m_nHeight = this_mipmap.height;
                        srcMipLevel.m_dwLinearSize = this_mipmap.size;
                        srcMipLevel.m_pbData = this_mipmap.data_uptr.get();

                        {
                            CMP_ERROR cmp_status;
                            cmp_status = CMP_ProcessTexture(&srcTexture, &dstTexture, kernel_options, nullptr);
                            assert(cmp_status == CMP_OK);
                        }

                        CMP_SaveTexture(this_mipmap_filename.c_str(), &dstTexture);

                        this_compressed_mipmap = dstTexture;
                    }

                    garbage_collector.emplace_back((*this_compressed_mipmap.m_pMipLevelTable)->m_pbData);

                    compressed_mipmaps_raw_data.emplace_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                                                            static_cast<uint32_t>(this_mipmap_level),
                                                                                                            (*this_compressed_mipmap.m_pMipLevelTable)->m_pbData,
                                                                                                            (*this_compressed_mipmap.m_pMipLevelTable)->m_dwLinearSize,
                                                                                                            (*this_compressed_mipmap.m_pMipLevelTable)->m_dwLinearSize / (*this_compressed_mipmap.m_pMipLevelTable)->m_nHeight));
                }
            }

            Anvil::ImageUniquePtr image_ptr;
            Anvil::ImageViewUniquePtr image_view_ptr;

            Anvil::Format texture_target_compressed_format = vulkanFormatToCompressedFormat_map.find(original_info.aligned_image_vulkan_format)->second;

            {
                auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(device_ptr,
                                                                            Anvil::ImageType::_2D,
                                                                            texture_target_compressed_format,
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
                                                                             texture_target_compressed_format,
                                                                             Anvil::ComponentSwizzle::R,
                                                                             (original_info.defaultCompCount >= 2)
                                                                             ? Anvil::ComponentSwizzle::G
                                                                             : Anvil::ComponentSwizzle::ZERO,
                                                                             (original_info.defaultCompCount >= 3)
                                                                             ? Anvil::ComponentSwizzle::B
                                                                             : Anvil::ComponentSwizzle::ZERO,
                                                                             (original_info.defaultCompCount >= 4)
                                                                             ? Anvil::ComponentSwizzle::A
                                                                             : Anvil::ComponentSwizzle::ONE);

                image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr));
            }

            images_uptrs.emplace_back(std::move(image_ptr));
            imagesViews_upts.emplace_back(std::move(image_view_ptr));

            imagesByglTFindex_to_imagesByFlashedIndex_umap.emplace(thisModel_imagesglTFcountSoFar, thisModel_imagesFlashedSoFar);
            thisModel_imagesFlashedSoFar++;
        }

        thisModel_imagesglTFcountSoFar++;
    }

    mipmapsGenerator_ptr->Reset();

    for (const tinygltf::Texture& this_texture : in_model.textures)
    {
        TextureInfo this_texture_info;

        auto search_imagesFlashed_index = imagesByglTFindex_to_imagesByFlashedIndex_umap.find(this_texture.source);
        if (search_imagesFlashed_index != imagesByglTFindex_to_imagesByFlashedIndex_umap.end())
        {
            this_texture_info.imageIndex  = imagesFlashedSoFar + search_imagesFlashed_index->second;
        }
        else
        {
            this_texture_info.imageIndex = -1; // won't be used because materials will use occlusion map via metallicRoughness
        }
         
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


    imagesFlashedSoFar += thisModel_imagesFlashedSoFar;
    imagesglTFcountSoFar += thisModel_imagesglTFcountSoFar;
    
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
    TextureInfo& this_textureInfo = texturesInfos[in_texture_index];
    assert(this_textureInfo.imageIndex != -1);

    return  imagesViews_upts[this_textureInfo.imageIndex].get();
}

Anvil::Sampler* TexturesOfMaterials::GetSampler(size_t in_texture_index)
{
    TextureInfo& this_textureInfo = texturesInfos[in_texture_index];
    assert(this_textureInfo.imageIndex != -1);

    return  samplers_uptrs[this_textureInfo.samplerIndex].get();
}