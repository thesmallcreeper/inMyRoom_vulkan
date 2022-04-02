#include "Graphics/Textures/ColorImage.h"

#include "glm/vec4.hpp"
#include "glm/vec3.hpp"

ColorImage::ColorImage(const tinygltf::Image* gltf_image_ptr,
                       std::string identifier_string,
                       std::string model_folder,
                       glTFsamplerWrap wrap_S,
                       glTFsamplerWrap wrap_T,
                       float filter_sigma)
        : TextureImage(gltf_image_ptr,
                       identifier_string,
                       model_folder,
                       wrap_S, wrap_T,
                       true,
                       false),
          filterSigma(filter_sigma)
{
}

ImageData ColorImage::CreateMipmap(const ImageData &reference, size_t dimension_factor)
{
    if (dimension_factor == 1)
        return reference;

    ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                               reference.GetComponentsCount(), reference.GetWrapS(), reference.GetWrapT());

    // Gaussian filter
    assert(dimension_factor == 2);
    float sigma = filterSigma;
    for (int x = 0; x != mipmap_imageData.GetWidth(); ++x) {
        for (int y = 0; y != mipmap_imageData.GetWidth(); ++y) {
            glm::vec3 color_sum = {0.f, 0.f, 0.f};
            glm::vec3 color_sum_unweighted = {0.f, 0.f, 0.f};
            float color_factor_sum = 0.f;
            float factor_sum = 0.f;
            for (int i = 0; i != 3; ++i) {
                for (int j = 0; j != 3; ++j) {
                    glm::vec4 sample_top_right = {
                            reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 0),
                            reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 1),
                            reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 2),
                            reference.GetComponent(2 * x + i + 1, 2 * y + j + 1, 3)
                    };
                    glm::vec4 sample_top_left = {
                            reference.GetComponent(2 * x - i, 2 * y + j + 1, 0),
                            reference.GetComponent(2 * x - i, 2 * y + j + 1, 1),
                            reference.GetComponent(2 * x - i, 2 * y + j + 1, 2),
                            reference.GetComponent(2 * x - i, 2 * y + j + 1, 3)
                    };
                    glm::vec4 sample_bottom_right = {
                            reference.GetComponent(2 * x + i + 1, 2 * y - j, 0),
                            reference.GetComponent(2 * x + i + 1, 2 * y - j, 1),
                            reference.GetComponent(2 * x + i + 1, 2 * y - j, 2),
                            reference.GetComponent(2 * x + i + 1, 2 * y - j, 3)
                    };
                    glm::vec4 sample_bottom_left = {
                            reference.GetComponent(2 * x - i, 2 * y - j, 0),
                            reference.GetComponent(2 * x - i, 2 * y - j, 1),
                            reference.GetComponent(2 * x - i, 2 * y - j, 2),
                            reference.GetComponent(2 * x - i, 2 * y - j, 3)
                    };

                    float factor = GaussianFilterFactor( float(i) + 0.5f, float(j) + 0.5f, sigma);

                    color_sum += glm::vec3(sample_top_right * sample_top_right.a * factor);
                    color_sum_unweighted += glm::vec3(sample_top_right * factor);
                    color_factor_sum += sample_top_right.a * factor;

                    color_sum += glm::vec3(sample_top_left * sample_top_left.a * factor);
                    color_sum_unweighted += glm::vec3(sample_top_left * factor);
                    color_factor_sum += sample_top_left.a * factor;

                    color_sum += glm::vec3(sample_bottom_right * sample_bottom_right.a * factor);
                    color_sum_unweighted += glm::vec3(sample_bottom_right * factor);
                    color_factor_sum += sample_bottom_right.a * factor;

                    color_sum += glm::vec3(sample_bottom_left * sample_bottom_left.a * factor);
                    color_sum_unweighted += glm::vec3(sample_bottom_left * factor);
                    color_factor_sum += sample_bottom_left.a * factor;

                    factor_sum += 4.f * factor;
                }
            }

            glm::vec3 color = color_sum / color_factor_sum;
            float alpha = color_factor_sum / factor_sum;
            if (alpha == 0.f) color = color_sum_unweighted / factor_sum;
            glm::vec4 result = glm::vec4(color, alpha);

            mipmap_imageData.SetComponent(x, y, 0, result.x);
            mipmap_imageData.SetComponent(x, y, 1, result.y);
            mipmap_imageData.SetComponent(x, y, 2, result.z);
            mipmap_imageData.SetComponent(x, y, 3, result.w);
        }
    }

    return mipmap_imageData;
}