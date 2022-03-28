#include "Graphics/Textures/NormalImage.h"

#include "glm/matrix.hpp"

NormalImage::NormalImage(const tinygltf::Image* gltf_image_ptr,
                         std::string identifier_string,
                         std::string model_folder,
                         glTFsamplerWrap wrap_S,
                         glTFsamplerWrap wrap_T,
                         float in_scale)
        : TextureImage(gltf_image_ptr,
                       identifier_string,
                       model_folder,
                       wrap_S, wrap_T,
                       false,
                       true),
          scale(in_scale)
{
}

ImageData NormalImage::CreateMipmap(const ImageData &reference, size_t dimension_factor)
{
    if (dimension_factor == 1)
        return reference;

    assert(reference.GetComponentsCount() == 4);
    ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                               4, reference.GetWrapS(), reference.GetWrapT());

    ImageData length_data(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                          1, reference.GetWrapS(), reference.GetWrapT());

    // Box filter
    for (size_t x = 0; x != mipmap_imageData.GetWidth(); ++x) {
        for (size_t y = 0; y != mipmap_imageData.GetWidth(); ++y) {
            glm::vec3 sum_value = glm::vec3(0.f, 0.f, 0.f);
            for (size_t i = 0; i != dimension_factor; ++i) {
                for (size_t j = 0; j != dimension_factor; ++j) {
                    glm::vec3 this_value;
                    this_value.x = reference.GetComponent(int(x*dimension_factor + i), int(y*dimension_factor + j),
                                                          0);
                    this_value.y = reference.GetComponent(int(x*dimension_factor + i), int(y*dimension_factor + j),
                                                          1);
                    this_value.z = reference.GetComponent(int(x*dimension_factor + i), int(y*dimension_factor + j),
                                                          2);

                    this_value = this_value * 2.f - 1.f;
                    this_value.x *= scale;
                    this_value.y *= scale;
                    sum_value += glm::normalize(this_value);
                }
            }

            glm::vec3 value_unormalized = sum_value / float(dimension_factor * dimension_factor);
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