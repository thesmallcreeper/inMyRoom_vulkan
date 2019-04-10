#include "MaterialsTextures.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

#include "DDS_Helpers.h"

#include <cstring>
#include <iostream>

MaterialsTextures::MaterialsTextures(tinygltf::Model& in_model, const std::string& in_imagesFolder, bool use_mipmaps,
                                     TexturesImagesUsage* in_texturesImagesUsage_ptr,
                                     Anvil::BaseDevice* const in_device_ptr)
{
    for (tinygltf::Image& thisImage : in_model.images)
    {
        std::unique_ptr<uint8_t[]> local_original_image_buffer;

        int32_t width;
        int32_t height;
        int32_t imageCompCount;

        fs::path path_to_mipmap_folder;

        if (!thisImage.uri.empty())
        {
            fs::path path_to_original_image = in_imagesFolder + "//" + thisImage.uri;
            path_to_mipmap_folder = in_imagesFolder + "//" + thisImage.uri.substr(0, thisImage.uri.find_last_of('.')) +
                "_mipmaps";

            fs::path absolute_path_to_original_image = absolute(path_to_original_image);

            unsigned char* stbi_data = stbi_load(absolute_path_to_original_image.generic_string().c_str(), &width,
                                                 &height, &imageCompCount, 0);
            assert(stbi_data);

            local_original_image_buffer.reset(new uint8_t[width * height * imageCompCount]);

            std::memcpy(local_original_image_buffer.get(), stbi_data, width * height * imageCompCount);

            stbi_image_free(stbi_data);
        }
        else
        {
            assert(0);
        }

        create_directory(path_to_mipmap_folder);

        Anvil::Format original_image_vulkan_format = componentsCountToVulkanFormat_map.find(imageCompCount)->second;
        CMP_FORMAT original_image_compressonator_format = vulkanFormatToCompressonatorFormat_map
                                                          .find(original_image_vulkan_format)->second;

        CMP_Texture srcTexture;

        srcTexture.dwSize = sizeof(srcTexture);
        srcTexture.dwDataSize = width * height * imageCompCount;
        srcTexture.dwPitch = width * imageCompCount;
        srcTexture.dwWidth = width;
        srcTexture.dwHeight = height;
        srcTexture.format = original_image_compressonator_format;
        srcTexture.pData = local_original_image_buffer.get();

        std::vector<CMP_Texture> mipmaps;

        CMP_CompressOptions options = {0};
        options.dwSize = sizeof(options);
        options.fquality = 0.1f;
        options.dwnumThreads = 8;

        std::vector<Anvil::MipmapRawData> mipmaps_raw_data;

        size_t mipmap_levels;
        if (use_mipmaps)
        {
            if (width >= height)
                mipmap_levels = static_cast<size_t>(std::floor(std::log2(width))) - 1;
            else
                mipmap_levels = static_cast<size_t>(std::floor(std::log2(height))) - 1;
        }
        else
            mipmap_levels = 1;

        uint32_t this_mipmap_width = width;
        uint32_t this_mipmap_height = height;

        for (size_t this_mipmap_level = 0; this_mipmap_level < mipmap_levels; this_mipmap_level++)
        {
            std::string this_mipmap_filename = path_to_mipmap_folder.string() + "//mipmap_" + std::to_string(
                this_mipmap_level) + ".DDS";
            CMP_Texture this_mipmap;

            if (!LoadDDSFile(this_mipmap_filename.c_str(), this_mipmap))
            {
                std::cout << thisImage.uri << " , creating mipmap level: " << this_mipmap_level << " width= " <<
                    this_mipmap_width << " height= " << this_mipmap_height << "\n";

                CMP_Texture new_mipmap;

                new_mipmap.dwSize = sizeof(new_mipmap);
                new_mipmap.dwWidth = this_mipmap_width;
                new_mipmap.dwHeight = this_mipmap_height;
                new_mipmap.dwPitch = 0;
                new_mipmap.format = vulkanFormatToCompressonatorFormat_map.find(image_preferred_format)->second;
                new_mipmap.dwDataSize = CMP_CalculateBufferSize(&new_mipmap);
                new_mipmap.pData = (CMP_BYTE*)malloc(new_mipmap.dwDataSize);

                CMP_ERROR cmp_status;
                cmp_status = CMP_ConvertTexture(&srcTexture, &new_mipmap, &options, nullptr, NULL, NULL);

                if (cmp_status != CMP_OK)
                    assert(0);

                SaveDDSFile(this_mipmap_filename.c_str(), new_mipmap);

                this_mipmap = new_mipmap;
            }

            mipmaps.emplace_back(this_mipmap);

            mipmaps_raw_data.emplace_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(
                Anvil::ImageAspectFlagBits::COLOR_BIT,
                static_cast<uint32_t>(this_mipmap_level),
                this_mipmap.pData,
                this_mipmap.dwDataSize,
                this_mipmap.dwDataSize / this_mipmap_height));

            if (this_mipmap_width > 1) this_mipmap_width /= 2;
            if (this_mipmap_height > 1) this_mipmap_height /= 2;
        }

        Anvil::ImageUniquePtr image_ptr;
        Anvil::ImageViewUniquePtr image_view_ptr;

        {
            auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(in_device_ptr,
                                                                        Anvil::ImageType::_2D,
                                                                        image_preferred_format,
                                                                        Anvil::ImageTiling::OPTIMAL,
                                                                        Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT |
                                                                        Anvil::ImageUsageFlagBits::SAMPLED_BIT,
                                                                        static_cast<uint32_t>(width),
                                                                        static_cast<uint32_t>(height),
                                                                        1, /* base_mipmap_depth */
                                                                        1, /* n_layers */
                                                                        Anvil::SampleCountFlagBits::_1_BIT,
                                                                        Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                        Anvil::SharingMode::EXCLUSIVE,
                                                                        use_mipmaps, /* in_use_full_mipmap_chain */
                                                                        Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT,
                                                                        Anvil::ImageCreateFlagBits::NONE,
                                                                        Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                                        /* in_final_image_layout    */
                                                                        &mipmaps_raw_data);
            /* in_mipmaps_ptr           */

            image_ptr = Anvil::Image::create(std::move(create_info_ptr));
        }

        for (auto this_data : mipmaps)
            delete this_data.pData;

        {
            auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(in_device_ptr,
                                                                         image_ptr.get(),
                                                                         0, /* n_base_layer        */
                                                                         0, /* n_base_mipmap_level */
                                                                         mipmap_levels, /* n_mipmaps           */
                                                                         Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                         image_preferred_format,
                                                                         Anvil::ComponentSwizzle::R,
                                                                         (imageCompCount >= 2)
                                                                             ? Anvil::ComponentSwizzle::G
                                                                             : Anvil::ComponentSwizzle::ZERO,
                                                                         (imageCompCount >= 3)
                                                                             ? Anvil::ComponentSwizzle::B
                                                                             : Anvil::ComponentSwizzle::ZERO,
                                                                         (imageCompCount >= 4)
                                                                             ? Anvil::ComponentSwizzle::A
                                                                             : Anvil::ComponentSwizzle::ONE);

            image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr));
        }

        images_uptrs.emplace_back(std::move(image_ptr));
        imagesViews_upts.emplace_back(std::move(image_view_ptr));
    }

    {
        // Default sampler because ffs glTF specs has autism
        Anvil::SamplerUniquePtr image_sampler_ptr;

        auto create_info_ptr = Anvil::SamplerCreateInfo::create(in_device_ptr,
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

    for (tinygltf::Sampler& thisSampler : in_model.samplers)
    {
        Anvil::SamplerUniquePtr image_sampler_ptr;

        auto create_info_ptr = Anvil::SamplerCreateInfo::create(in_device_ptr,
                                                                glTFsamplerMagFilterToFilter_map
                                                                .find(static_cast<glTFsamplerMagFilter>(thisSampler.
                                                                    magFilter))->second,
                                                                glTFsamplerMinFilterToFilterAndMipmapMode_map
                                                                .find(static_cast<glTFsamplerMinFilter>(thisSampler.
                                                                    magFilter))->second.first,
                                                                glTFsamplerMinFilterToFilterAndMipmapMode_map
                                                                .find(static_cast<glTFsamplerMinFilter>(thisSampler.
                                                                    magFilter))->second.second,
                                                                glTFsamplerWrapToAddressMode_map
                                                                .find(static_cast<glTFsamplerWrap>(thisSampler.wrapS))->
                                                                second,
                                                                glTFsamplerWrapToAddressMode_map
                                                                .find(static_cast<glTFsamplerWrap>(thisSampler.wrapT))->
                                                                second,
                                                                Anvil::SamplerAddressMode::REPEAT,
                                                                0.0f, /* in_lod_bias        */
                                                                16.0f, /* in_max_anisotropy  */
                                                                false, /* in_compare_enable  */
                                                                Anvil::CompareOp::NEVER, /* in_compare_enable  */
                                                                0.0f, /* in_min_lod         */
                                                                16.0f, /* in_min_lod         */
                                                                Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                                false); /* in_use_unnormalized_coordinates */

        image_sampler_ptr = Anvil::Sampler::create(std::move(create_info_ptr));

        samplers_uptrs.emplace_back(std::move(image_sampler_ptr));
    }

    for (tinygltf::Texture& thisTexture : in_model.textures)
    {
        TextureInfo thisTextureInfo;
        thisTextureInfo.imageIndex = thisTexture.source;
        if (thisTexture.sampler != -1)
            thisTextureInfo.samplerIndex = thisTexture.sampler + 1;
        else
            thisTextureInfo.samplerIndex = 0;

        texturesInfos.emplace_back(thisTextureInfo);
    }
}

MaterialsTextures::~MaterialsTextures()
{
    samplers_uptrs.clear();
    imagesViews_upts.clear();
    images_uptrs.clear();
}
