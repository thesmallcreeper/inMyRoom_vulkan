#include "Graphics/Textures/NormalImage.h"

#include "glm/matrix.hpp"

NormalImage::NormalImage(const tinygltf::Image* gltf_image_ptr,
                         std::string identifier_string,
                         std::string model_folder,
                         glTFsamplerWrap wrap_S,
                         glTFsamplerWrap wrap_T,
                         float in_scale,
                         float filter_sigma)
        : TextureImage(gltf_image_ptr,
                       identifier_string,
                       model_folder,
                       wrap_S, wrap_T,
                       false,
                       true),
          scale(in_scale),
          filterSigma(filter_sigma)
{
}

ImageData NormalImage::CreateMipmap(const ImageData &reference, size_t dimension_factor)
{
    if (dimension_factor == 1) {
        ImageData length_data(reference.GetWidth(), reference.GetHeight(),
                              1, reference.GetWrapS(), reference.GetWrapT());
        length_data.BiasComponents({1.f});
        widthToLengthsData.emplace(length_data.GetWidth(), std::move(length_data));

        return reference;
    } else {
        assert(reference.GetComponentsCount() == 4);
        ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                                   4, reference.GetWrapS(), reference.GetWrapT());

        ImageData length_data(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                              1, reference.GetWrapS(), reference.GetWrapT());

        // Gaussian filter
        assert(dimension_factor == 2);
        float sigma = filterSigma;
        for (int x = 0; x != mipmap_imageData.GetWidth(); ++x) {
            for (int y = 0; y != mipmap_imageData.GetWidth(); ++y) {
                glm::vec3 sum_value = glm::vec3(0.f, 0.f, 0.f);
                float factor_sum = 0.f;
                for (int i = 0; i != 3; ++i) {
                    for (int j = 0; j != 3; ++j) {
                        glm::vec3 sample_top_right = {
                                reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 0),
                                reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 1),
                                reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 2)};
                        sample_top_right = sample_top_right * 2.f - 1.f;
                        sample_top_right.x *= scale;
                        sample_top_right.y *= scale;

                        glm::vec3 sample_top_left = {
                                reference.GetComponent(2 * x - i, 2 * y + j + 1, 0),
                                reference.GetComponent(2 * x - i, 2 * y + j + 1, 1),
                                reference.GetComponent(2 * x - i, 2 * y + j + 1, 2)};
                        sample_top_left = sample_top_left * 2.f - 1.f;
                        sample_top_left.x *= scale;
                        sample_top_left.y *= scale;

                        glm::vec3 sample_bottom_right = {
                                reference.GetComponent(2 * x + i + 1, 2 * y - j, 0),
                                reference.GetComponent(2 * x + i + 1, 2 * y - j, 1),
                                reference.GetComponent(2 * x + i + 1, 2 * y - j, 2)
                        };
                        sample_bottom_right = sample_bottom_right * 2.f - 1.f;
                        sample_bottom_right.x *= scale;
                        sample_bottom_right.y *= scale;

                        glm::vec3 sample_bottom_left = {
                                reference.GetComponent(2 * x - i, 2 * y - j, 0),
                                reference.GetComponent(2 * x - i, 2 * y - j, 1),
                                reference.GetComponent(2 * x - i, 2 * y - j, 2)
                        };
                        sample_bottom_left = sample_bottom_left * 2.f - 1.f;
                        sample_bottom_left.x *= scale;
                        sample_bottom_left.y *= scale;

                        float factor = GaussianFilterFactor(float(i) + 0.5f, float(j) + 0.5f, sigma);

                        sum_value += glm::normalize(sample_top_right) * factor;
                        sum_value += glm::normalize(sample_top_left) * factor;
                        sum_value += glm::normalize(sample_bottom_right) * factor;
                        sum_value += glm::normalize(sample_bottom_left) * factor;

                        factor_sum += 4.f * factor;
                    }
                }
                glm::vec3 value_unormalized = sum_value / factor_sum;
                float length = glm::length(value_unormalized);

                glm::vec3 value = glm::normalize(value_unormalized);
                value.x /= scale;
                value.y /= scale;
                value = (value + 1.f) / 2.f;

                mipmap_imageData.SetComponent(x, y, 0, value.x);
                mipmap_imageData.SetComponent(x, y, 1, value.y);
                mipmap_imageData.SetComponent(x, y, 2, value.z);
                mipmap_imageData.SetComponent(x, y, 3, 1.f);

                length_data.SetComponent(x, y, 0, length);
            }
        }

        widthToLengthsData.emplace(length_data.GetWidth(), std::move(length_data));
        return mipmap_imageData;
    }
}

const std::unordered_map<uint32_t, ImageData> &NormalImage::GetWidthToLengthsDataUmap() const
{
    return widthToLengthsData;
}
