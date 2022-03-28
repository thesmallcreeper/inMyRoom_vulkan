#include "Graphics/Textures/MetallicRoughnessImage.h"

MetallicRoughnessImage::MetallicRoughnessImage(const tinygltf::Image* gltf_image_ptr,
                                               std::string identifier_string,
                                               std::string model_folder,
                                               glTFsamplerWrap wrap_S,
                                               glTFsamplerWrap wrap_T,
                                               float in_metallic_factor,
                                               float in_roughness_factor,
                                               const std::unordered_map<uint32_t, ImageData> &in_widthToLengthsData)
        : TextureImage(gltf_image_ptr,
                       identifier_string,
                       model_folder,
                       wrap_S, wrap_T,
                       false,
                       true),
          metallic_factor(in_metallic_factor),
          roughness_factor(in_roughness_factor),
          widthToLengthsData(in_widthToLengthsData)
{
}

ImageData MetallicRoughnessImage::CreateMipmap(const ImageData &reference, size_t dimension_factor)
{
    if (dimension_factor == 0) {
        if (widthToLengthsData.empty()) {
            ImageData mipmap_imageData(4, 4, 4, wrap_S, wrap_T);
            mipmap_imageData.BiasComponents({1.f, roughness_factor, metallic_factor, 1.f});

            return mipmap_imageData;
        } else {
            const ImageData& biggest_length_image = std::max(widthToLengthsData.begin(), widthToLengthsData.end(),
                                                             [](const auto& lhs, const auto& rhs) {return lhs->second.GetWidth() < rhs->second.GetWidth();})->second;

            size_t width = biggest_length_image.GetWidth() * 2;
            size_t height = biggest_length_image.GetHeight() * 2;

            ImageData mipmap_imageData(width, height, 4, wrap_S, wrap_T);
            mipmap_imageData.BiasComponents({1.f, roughness_factor, metallic_factor, 1.f});

            return mipmap_imageData;
        }
    } else if (dimension_factor == 1) {
        assert(reference.GetComponentsCount() == 4);

        ImageData mipmap_imageData(reference);
        mipmap_imageData.ScaleComponents({1.f, roughness_factor, metallic_factor, 1.f});

        return mipmap_imageData;
    } else {
        assert(reference.GetComponentsCount() == 4);
        ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                                   4, reference.GetWrapS(), reference.GetWrapT());

        bool should_apply_roughness_normal_correction = widthToLengthsData.size() && widthToLengthsData.find(mipmap_imageData.GetWidth()) != widthToLengthsData.end();

        for (size_t x = 0; x != mipmap_imageData.GetWidth(); ++x) {
            for (size_t y = 0; y != mipmap_imageData.GetWidth(); ++y) {
                float roughness_sum = 0.f;
                float metallic_sum = 0.f;
                for (size_t i = 0; i != dimension_factor; ++i) {
                    for (size_t j = 0; j != dimension_factor; ++j) {
                        roughness_sum += reference.GetComponent(int(x * dimension_factor + i), int(y * dimension_factor + j),
                                                                1);
                        metallic_sum += reference.GetComponent(int(x * dimension_factor + i), int(y * dimension_factor + j),
                                                               2);
                    }
                }

                float metallic_value = metallic_sum / float(dimension_factor * dimension_factor);
                float roughness_value = roughness_sum / float(dimension_factor * dimension_factor);

                // TODO: roughness correction

                mipmap_imageData.SetComponent(x, y, 0, 1.f);
                mipmap_imageData.SetComponent(x, y, 1, roughness_value);
                mipmap_imageData.SetComponent(x, y, 2, metallic_value);
                mipmap_imageData.SetComponent(x, y, 3, 1.f);
            }
        }

        return mipmap_imageData;
    }
}