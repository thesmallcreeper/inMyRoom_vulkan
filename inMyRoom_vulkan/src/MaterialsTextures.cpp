#include "MaterialsTextures.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"

MaterialsTextures::MaterialsTextures(tinygltf::Model& in_model, const std::string& in_imagesFolder, Anvil::BaseDevice* in_device_ptr)
{
    for (tinygltf::Image& thisImage : in_model.images)
    {
        if (thisImage.uri.size())
        {
            std::experimental::filesystem::path pathToImage = in_imagesFolder + "/" + thisImage.uri;
            std::experimental::filesystem::path adsolutePathToImage = std::experimental::filesystem::absolute(pathToImage);

            int32_t width;
            int32_t height;
            int32_t imageCompCount;
            unsigned char* stbi_data = stbi_load(adsolutePathToImage.generic_string().c_str(), &width, &height, &imageCompCount, 0);
            assert(stbi_data);

            int32_t compCount;
            if (imageCompCount == 3)
                compCount = 4;
            else
                compCount = imageCompCount;

            std::unique_ptr<uint8_t[]> localImageBuffer (new uint8_t[width*height*compCount]);

            if (imageCompCount == 3)
            {
                for(size_t i=0; i < width*height*compCount; i++)
                {
                    if (i % 4 == 3)
                        localImageBuffer[i] = 0xff;
                    else
                        localImageBuffer[i] = stbi_data[i - i / 4];
                }

            }
            else
            {
                for (size_t i = 0; i < width*height*compCount; i++)
                    localImageBuffer[i] = stbi_data[i];
                compCount = imageCompCount;
            }


            std::vector<Anvil::MipmapRawData> ImageMipmapsRawData;

            ImageMipmapsRawData.emplace_back(Anvil::MipmapRawData::create_2D_from_uchar_ptr(Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                                            static_cast<uint32_t>(0),
                                                                                            localImageBuffer.get(),
                                                                                            static_cast<uint32_t>(width*height*compCount),
                                                                                            static_cast<uint32_t>(width*compCount)));

            Anvil::ImageUniquePtr image_ptr;
            Anvil::ImageViewUniquePtr image_view_ptr;

            Anvil::Format imageFormat;
            {
                auto search = componentsCountToFormat_map.find(compCount);
                imageFormat = search->second;
            }

            {
                auto create_info_ptr = Anvil::ImageCreateInfo::create_alloc(in_device_ptr,
                                                                            Anvil::ImageType::_2D,
                                                                            imageFormat,
                                                                            Anvil::ImageTiling::OPTIMAL,
                                                                            Anvil::ImageUsageFlagBits::TRANSFER_DST_BIT | Anvil::ImageUsageFlagBits::SAMPLED_BIT,
                                                                            static_cast<uint32_t>(width),
                                                                            static_cast<uint32_t>(height),
                                                                            1,                                                            /* base_mipmap_depth */
                                                                            1,                                                            /* n_layers */
                                                                            Anvil::SampleCountFlagBits::_1_BIT,
                                                                            Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                            Anvil::SharingMode::EXCLUSIVE,
                                                                            false,                                                        /* in_use_full_mipmap_chain */
                                                                            Anvil::MemoryFeatureFlagBits::DEVICE_LOCAL_BIT,
                                                                            Anvil::ImageCreateFlagBits::NONE,
                                                                            Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,                 /* in_final_image_layout    */
                                                                            &ImageMipmapsRawData);                                        /* in_mipmaps_ptr           */

                image_ptr = Anvil::Image::create(std::move(create_info_ptr));
            }

            stbi_image_free(stbi_data);

            {
                auto create_info_ptr = Anvil::ImageViewCreateInfo::create_2D(in_device_ptr,
                                                                             image_ptr.get(),
                                                                             0,                                                           /* n_base_layer        */
                                                                             0,                                                           /* n_base_mipmap_level */
                                                                             1,                                                           /* n_mipmaps           */
                                                                             Anvil::ImageAspectFlagBits::COLOR_BIT,
                                                                             imageFormat,
                                                                             Anvil::ComponentSwizzle::R,
                                                                             (imageCompCount >= 2) ? Anvil::ComponentSwizzle::G : Anvil::ComponentSwizzle::ZERO,
                                                                             (imageCompCount >= 3) ? Anvil::ComponentSwizzle::B : Anvil::ComponentSwizzle::ZERO,
                                                                             (imageCompCount >= 4) ? Anvil::ComponentSwizzle::A : Anvil::ComponentSwizzle::ONE);

                image_view_ptr = Anvil::ImageView::create(std::move(create_info_ptr));
            }

            images.emplace_back(std::move(image_ptr));
            imagesViews.emplace_back(std::move(image_view_ptr));

        }

    }

    {       // Default sampler because ffs glTF specs has autism
        Anvil::SamplerUniquePtr image_sampler_ptr;

        auto create_info_ptr = Anvil::SamplerCreateInfo::create(in_device_ptr,
                                                                Anvil::Filter::LINEAR,
                                                                Anvil::Filter::LINEAR,
                                                                Anvil::SamplerMipmapMode::LINEAR,
                                                                Anvil::SamplerAddressMode::REPEAT,
                                                                Anvil::SamplerAddressMode::REPEAT,
                                                                Anvil::SamplerAddressMode::REPEAT,
                                                                0.0f,                                                                 /* in_lod_bias        */
                                                                16.0f,                                                                 /* in_max_anisotropy  */
                                                                false,                                                                /* in_compare_enable  */
                                                                Anvil::CompareOp::NEVER,                                              /* in_compare_enable  */
                                                                0.0f,                                                                 /* in_min_lod         */
                                                                0.0f,                                                                 /* in_max_lod         */
                                                                Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                                false);                                                               /* in_use_unnormalized_coordinates */

        image_sampler_ptr = Anvil::Sampler::create(std::move(create_info_ptr));

        samplers.emplace_back(std::move(image_sampler_ptr));

    }

    for (tinygltf::Sampler& thisSampler : in_model.samplers)
    {
        Anvil::SamplerUniquePtr image_sampler_ptr; 

        auto create_info_ptr = Anvil::SamplerCreateInfo::create(in_device_ptr,
                                                                glTFsamplerMagFilterToFilter_map.find(static_cast<glTFsamplerMagFilter>(thisSampler.magFilter))->second,
                                                                glTFsamplerMinFilterToFilterAndMipmapMode_map.find(static_cast<glTFsamplerMinFilter>(thisSampler.magFilter))->second.first,
                                                                glTFsamplerMinFilterToFilterAndMipmapMode_map.find(static_cast<glTFsamplerMinFilter>(thisSampler.magFilter))->second.second,
                                                                glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(thisSampler.wrapS))->second,
                                                                glTFsamplerWrapToAddressMode_map.find(static_cast<glTFsamplerWrap>(thisSampler.wrapT))->second,
                                                                Anvil::SamplerAddressMode::REPEAT,
                                                                0.0f,                                                                 /* in_lod_bias        */
                                                                16.0f,                                                                 /* in_max_anisotropy  */
                                                                false,                                                                /* in_compare_enable  */
                                                                Anvil::CompareOp::NEVER,                                              /* in_compare_enable  */
                                                                0.0f,                                                                 /* in_min_lod         */
                                                                0.0f,                                                                 /* in_min_lod         */
                                                                Anvil::BorderColor::INT_OPAQUE_BLACK,
                                                                false);                                                               /* in_use_unnormalized_coordinates */

        image_sampler_ptr = Anvil::Sampler::create(std::move(create_info_ptr));

        samplers.emplace_back(std::move(image_sampler_ptr));
    }

    for (tinygltf::Texture& thisTexture : in_model.textures)
    {
        TextureInfo thisTextureInfo;
        thisTextureInfo.imageIndex = thisTexture.source;
        if (thisTexture.sampler != -1)
            thisTextureInfo.samplerIndex = thisTexture.sampler + 1;
        else
            thisTextureInfo.samplerIndex = 0;

        textures.emplace_back(thisTextureInfo);
    }
}

MaterialsTextures::~MaterialsTextures()
{
    samplers.clear();
    imagesViews.clear();
    images.clear();
}
