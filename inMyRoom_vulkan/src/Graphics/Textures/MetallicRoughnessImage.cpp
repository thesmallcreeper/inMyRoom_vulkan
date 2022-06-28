#include "Graphics/Textures/MetallicRoughnessImage.h"

#include "common/RoughnessLengthMap.h"

MetallicRoughnessImage::MetallicRoughnessImage(const tinygltf::Image* gltf_image_ptr,
                                               std::string identifier_string,
                                               std::string model_folder,
                                               glTFsamplerWrap wrap_S,
                                               glTFsamplerWrap wrap_T,
                                               float in_metallic_factor,
                                               float in_roughness_factor,
                                               const std::unordered_map<uint32_t, ImageData> &in_widthToLengthsData,
                                               float filter_sigma)
        : TextureImage(gltf_image_ptr,
                       identifier_string,
                       model_folder,
                       wrap_S, wrap_T,
                       false,
                       true),
          metallic_factor(in_metallic_factor),
          roughness_factor(in_roughness_factor),
          widthToLengthsData(in_widthToLengthsData),
          filterSigma(filter_sigma)
{
}

ImageData MetallicRoughnessImage::CreateMipmap(const ImageData &reference, size_t dimension_factor)
{
    if (dimension_factor == 0) {
        if (widthToLengthsData.empty()) {
            ImageData mipmap_imageData(4, 4, 4, wrap_S, wrap_T);
            float roughness = std::max(TEX_FILTERING_MIN_ROUGH, roughness_factor);
            float metallic = metallic_factor;
            mipmap_imageData.BiasComponents({1.f, roughness, metallic, 1.f});

            return mipmap_imageData;
        } else {
            const ImageData& biggest_length_image = std::max(widthToLengthsData.begin(), widthToLengthsData.end(),
                                                             [](const auto& lhs, const auto& rhs) {return lhs->second.GetWidth() < rhs->second.GetWidth();})->second;

            size_t width = biggest_length_image.GetWidth();
            size_t height = biggest_length_image.GetHeight();

            ImageData mipmap_imageData(width, height, 4, wrap_S, wrap_T);
            float roughness = std::max(TEX_FILTERING_MIN_ROUGH, roughness_factor);
            float metallic = metallic_factor;
            mipmap_imageData.BiasComponents({1.f, roughness, metallic, 1.f});

            return mipmap_imageData;
        }
    } else if (dimension_factor == 1) {
        assert(reference.GetComponentsCount() == 4);

        ImageData mipmap_imageData(reference);
        mipmap_imageData.ScaleComponents({1.f, roughness_factor, metallic_factor, 1.f});
        mipmap_imageData.MaxComponents({1.f, TEX_FILTERING_MIN_ROUGH, 0.0f, 1.f});

        return mipmap_imageData;
    } else {
        assert(reference.GetComponentsCount() == 4);
        ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                                   4, reference.GetWrapS(), reference.GetWrapT());

        const ImageData* length_imageData_ptr = nullptr;
        bool should_apply_roughness_normal_correction = widthToLengthsData.size() && widthToLengthsData.find(mipmap_imageData.GetWidth()) != widthToLengthsData.end();
        if (should_apply_roughness_normal_correction) {
            length_imageData_ptr = &widthToLengthsData.find(mipmap_imageData.GetWidth())->second;
        }

        // Gaussian filter
        assert(dimension_factor == 2);
        float sigma = filterSigma;
        for (int x = 0; x != mipmap_imageData.GetWidth(); ++x) {
            for (int y = 0; y != mipmap_imageData.GetWidth(); ++y) {
                float roughnessLength_sum = 0.f;
                float metallic_sum = 0.f;
                float factor_sum = 0.f;
                for (int i = 0; i != 3; ++i) {
                    for (int j = 0; j != 3; ++j) {
                        float factor = GaussianFilterFactor( float(i) + 0.5f, float(j) + 0.5f, sigma);

                        roughnessLength_sum += RoughnessToLength(reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 1)) * factor;
                        metallic_sum += reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 2) * factor;

                        roughnessLength_sum += RoughnessToLength(reference.GetComponent(2 * x - i, 2 * y + j + 1, 1)) * factor;
                        metallic_sum += reference.GetComponent(2 * x - i, 2 * y + j + 1, 2) * factor;

                        roughnessLength_sum += RoughnessToLength(reference.GetComponent(2 * x + i + 1, 2 * y - j, 1)) * factor;
                        metallic_sum += reference.GetComponent(2 * x + i + 1, 2 * y - j, 2) * factor;

                        roughnessLength_sum += RoughnessToLength(reference.GetComponent(2 * x - i, 2 * y - j, 1)) * factor;
                        metallic_sum += reference.GetComponent(2 * x - i, 2 * y - j, 2) * factor;

                        factor_sum += 4.f * factor;
                    }
                }

                float metallic_value = metallic_sum / factor_sum;
                float roughnessLength_value = roughnessLength_sum / factor_sum;

                // Correcting roughness
                if (should_apply_roughness_normal_correction) {
                    float normal_length = length_imageData_ptr->GetComponent(x, y, 0);
                    roughnessLength_value = roughnessLength_value * normal_length;
                }
                mipmap_imageData.SetComponent(x, y, 0, 1.f);
                mipmap_imageData.SetComponent(x, y, 1, LengthToRoughness(roughnessLength_value));
                mipmap_imageData.SetComponent(x, y, 2, metallic_value);
                mipmap_imageData.SetComponent(x, y, 3, 1.f);
            }
        }

        return mipmap_imageData;
    }
}