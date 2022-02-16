#include "Graphics/ImagesInterfaces.h"

#include <limits>
#include <cmath>
#include <filesystem>
#include <iostream>

#include "glm/vec4.hpp"
#include "glm/vec3.hpp"
#include "glm/mat4x4.hpp"

#include "stb_image.h"
#include "stb_image_write.h"

ImageData::ImageData(size_t in_width, size_t in_height, size_t in_components,
                     glTFsamplerWrap in_wrap_S, glTFsamplerWrap in_wrap_T)
    : width(in_width),
      height(in_height),
      componentsCount(in_components),
      wrap_S(in_wrap_S),
      wrap_T(in_wrap_T)
{
    floatBuffer.resize(width * height * componentsCount);
}

void ImageData::SetImage(const std::vector<uint8_t> &data, bool is_srgb)
{
    assert(data.size() == width * height * componentsCount);

    for (size_t i = 0; i != width; ++i) {
        for (size_t j = 0; j != height; ++j) {
            for (size_t k = 0; k != componentsCount; ++k) {
                uint8_t this_data =  data[(j * width + i) * componentsCount + k];

                float value = 0.f;
                if (is_srgb) value = SRGBtoFloat(this_data);
                else value = R8toFloat(this_data);

                SetComponent(i, j, k, value);
            }
        }
    }
}

void ImageData::SetImage(const std::vector<uint16_t> &data)
{
    assert(data.size() == width * height * componentsCount);

    for (size_t i = 0; i != width; ++i) {
        for (size_t j = 0; j != height; ++j) {
            for (size_t k = 0; k != componentsCount; ++k) {
                uint16_t this_data =  data[(j * width + i) * componentsCount + k];

                float value = R16toFloat(this_data);
                SetComponent(i, j, k, value);
            }
        }
    }
}

std::vector<std::byte> ImageData::GetImage8BitPerChannel(bool srgb) const
{

    size_t buffer_size = width * height * componentsCount;

    std::vector<std::byte> buffer(buffer_size);
    auto buffer_ptr = reinterpret_cast<uint8_t*>(buffer.data());

    for (size_t i = 0; i != width; ++i) {
        for (size_t j = 0; j != height; ++j) {
            for (size_t k = 0; k != componentsCount; ++k) {
                float component = GetComponent(int(i), int(j), k);

                uint8_t data = 0;
                if (srgb) data = FloatToSRGB(component);
                else data = FloatToR8(component);

                buffer_ptr[(j * width + i) * componentsCount + k] = data;
            }
        }
    }

    return buffer;
}

std::vector<std::byte> ImageData::GetImage16BitPerChannel() const
{
    size_t buffer_size = width * height * componentsCount * 2;

    std::vector<std::byte> buffer(buffer_size);
    auto buffer_ptr = reinterpret_cast<uint16_t*>(buffer.data());

    for (size_t i = 0; i != width; ++i) {
        for (size_t j = 0; j != height; ++j) {
            for (size_t k = 0; k != componentsCount; ++k) {
                float component = GetComponent(int(i), int(j), k);

                uint16_t data = FloatToR16(component);
                buffer_ptr[(j * width + i) * componentsCount + k] = data;
            }
        }
    }

    return buffer;
}

std::vector<std::byte> ImageData::GetImageA2R10G10B10() const
{
    assert(componentsCount == 3 || componentsCount == 4);

    size_t buffer_byte_size = width * height * sizeof(uint32_t);

    std::vector<std::byte> buffer(buffer_byte_size);
    auto buffer_ptr = reinterpret_cast<uint32_t*>(buffer.data());

    for (size_t i = 0; i != width; ++i) {
        for (size_t j = 0; j != height; ++j) {
            uint32_t data_r, data_g, data_b, data_a;

            {
                float component_r = GetComponent(int(i), int(j), 0);
                data_r = FloatToRx(component_r, 0x03FF);
            } {
                float component_g = GetComponent(int(i), int(j), 1);
                data_g = FloatToRx(component_g, 0x03FF);
            } {
                float component_b = GetComponent(int(i), int(j), 2);
                data_b = FloatToRx(component_b, 0x03FF);
            } {
                if (componentsCount == 3) data_a = 0x0000;
                else {
                    float component_a = GetComponent(int(i), int(j), 3);
                    data_a = FloatToRx(component_a, 0x0003);
                }
            }

            buffer_ptr[(j * width + i)] = data_a << 30 | data_r << 20 | data_g << 10 | data_b ;
        }
    }

    return buffer;
}

float ImageData::GetComponent(int x, int y, size_t component) const
{
    assert(component < componentsCount);

    size_t x_wrapped = WrapAddress(wrap_S, width , x);
    size_t y_wrapped = WrapAddress(wrap_T, height, y);

    return floatBuffer[(y_wrapped * width + x_wrapped) * componentsCount + component];
}

void ImageData::SetComponent(size_t x, size_t y, size_t component, float value)
{
    assert(x < width);
    assert(y < height);
    assert(component < componentsCount);

    floatBuffer[(y * width + x) * componentsCount + component] = value;
}

//
// To "float"
float ImageData::SRGBtoFloat(uint8_t value)
{
    float normalized_value = R8toFloat(value);

    float linear_value = 0.f;
    if(normalized_value < 0.04045f)
        linear_value = normalized_value / 12.92f;
    else
        linear_value = std::pow((normalized_value + 0.055f) / 1.055f, 2.4f);

    return linear_value;
}

float ImageData::R8toFloat(uint8_t value)
{
    float float_value = value;
    float normalized_value = float_value / float(std::numeric_limits<uint8_t>::max());
    return normalized_value;
}

float ImageData::R16toFloat(uint16_t value)
{
    float float_value = value;
    float normalized_value = float_value / float(std::numeric_limits<uint16_t>::max());
    return normalized_value;
}

//
// From "float"
uint8_t ImageData::FloatToSRGB(float value)
{
    value = std::max(value, 0.f);
    value = std::min(value, 1.f);

    float srgb_value = 0.f;
    if (value < 0.0031308f)
        srgb_value = value * 12.92f;
    else
        srgb_value =  std::pow(value, 1.0f / 2.4f) * 1.055f - 0.055f;

    return FloatToR8(srgb_value);
}

uint8_t ImageData::FloatToR8(float value)
{
    return uint8_t(FloatToRx(value, std::numeric_limits<uint8_t>::max()));
}

uint16_t ImageData::FloatToR16(float value)
{
    return uint16_t(FloatToRx(value, std::numeric_limits<uint16_t>::max()));
}

uint32_t ImageData::FloatToRx(float value, uint32_t max)
{
    value = std::max(value, 0.f);
    value = std::min(value, 1.f);

    float denormalized_value = std::round(value * float(max));
    auto quantized_value = uint32_t(denormalized_value);
    return quantized_value;
}

size_t ImageData::WrapAddress(glTFsamplerWrap wrap, size_t size, int address)
{
    int return_address = 0;
    if(wrap == glTFsamplerWrap::repeat)
        return_address = address % int(size);
    else if (wrap == glTFsamplerWrap::clamp_to_edge) {
        address = std::min(address, int(size - 1));
        address = std::max(address, 0);
        return_address = address;
    }
    else if (wrap == glTFsamplerWrap::mirrored_repeat) {
        address = address % int(size * 2);

        if (address >= size) return_address = 2*int(size) - address;
        else return_address = address;
    }


    assert(return_address < size);
    assert(return_address >= 0);

    return size_t(return_address);
}

std::vector<float> ImageData::GetComponentsMax() const
{
    std::vector<float> max_values(componentsCount, -std::numeric_limits<float>::infinity());

    for(size_t i = 0; i != floatBuffer.size(); ++i) {
        size_t component = i % componentsCount;
        max_values[component] = std::max(floatBuffer[i], max_values[component]);
    }

    return max_values;
}

std::vector<float> ImageData::GetComponentsMin() const
{
    std::vector<float> min_values(componentsCount, +std::numeric_limits<float>::infinity());

    for(size_t i = 0; i != floatBuffer.size(); ++i) {
        size_t component = i % componentsCount;
        min_values[component] = std::min(floatBuffer[i], min_values[component]);
    }

    return min_values;
}

void ImageData::ScaleComponents(const std::vector<float> &scales)
{
    assert(scales.size() == componentsCount);

    for(size_t i = 0; i != floatBuffer.size(); ++i) {
        size_t component = i % componentsCount;
        floatBuffer[i] = floatBuffer[i] * scales[component];
    }
}

void ImageData::BiasComponents(const std::vector<float> &bias)
{
    assert(bias.size() == componentsCount);

    for(size_t i = 0; i != floatBuffer.size(); ++i) {
        size_t component = i % componentsCount;
        floatBuffer[i] = floatBuffer[i] + bias[component];
    }
}


TextureImage::TextureImage(const tinygltf::Image &gltf_image, std::string identifier_string, std::string model_folder,
                           glTFsamplerWrap in_wrap_S, glTFsamplerWrap in_wrap_T,
                           bool in_sRGB, bool in_saveAs16bit)
    : glTFimage_ptr(&gltf_image),
      identifierString(std::move(identifier_string)),
      modelFolder(std::move(model_folder)),
      wrap_S(in_wrap_S),
      wrap_T(in_wrap_T),
      sRGBifPossible(in_sRGB),
      saveAs16bit(in_saveAs16bit)
{
}

void TextureImage::RetrieveMipmaps(size_t min_x, size_t min_y)
{
    assert(glTFimage_ptr->width != -1);
    assert(glTFimage_ptr->height != -1);

    size_t this_mipmap_width = glTFimage_ptr->width;
    size_t this_mipmap_height = glTFimage_ptr->height;
    size_t this_mipmap_factor = 1;
    size_t this_mipmap_level = 0;

    // Get mipmaps
    do {
        auto mipmap_opt = LoadMipmapFromDisk(this_mipmap_level);
        if (mipmap_opt) {
            imagesData.emplace_back(mipmap_opt.value());
        } else {
            std::cout << "---Creating mipmap of: " + identifierString + " , level = " + std::to_string(this_mipmap_level) + "\n";
            if (this_mipmap_level == 0) {
                int width = glTFimage_ptr->width;
                int height = glTFimage_ptr->height;
                int componentsCount = (glTFimage_ptr->component == 3) ? 4 : glTFimage_ptr->component;

                ImageData this_image_data(width, height, componentsCount, wrap_S, wrap_T);
                if (glTFimage_ptr->bits == 8) {
                    std::vector<uint8_t> buffer;
                    for (size_t i = 0; i != glTFimage_ptr->image.size(); ++i) {
                        buffer.emplace_back(glTFimage_ptr->image[i]);
                        if (i % 3 == 2 && glTFimage_ptr->component == 3) {
                            buffer.emplace_back(std::numeric_limits<uint8_t>::max());
                        }
                    }
                    this_image_data.SetImage(buffer, sRGBifPossible);
                } else if (glTFimage_ptr->bits == 16) {
                    std::vector<uint16_t> buffer;
                    auto image_ptr = reinterpret_cast<const uint16_t*>(glTFimage_ptr->image.data());
                    for (size_t i = 0; i != glTFimage_ptr->image.size() / 2; ++i) {
                        buffer.emplace_back(image_ptr[i]);
                        if (i % 3 == 2 && glTFimage_ptr->component == 3) {
                            buffer.emplace_back(std::numeric_limits<uint16_t>::max());
                        }
                    }
                    this_image_data.SetImage(buffer);
                }

                imagesData.emplace_back(CreateMipmap(this_image_data, 1));
            } else {
                imagesData.emplace_back(CreateMipmap(imagesData[this_mipmap_level - 1], 2));
            }
        }

        this_mipmap_width /= 2;
        this_mipmap_height /= 2;
        this_mipmap_factor *= 2;
        ++this_mipmap_level;
    } while (this_mipmap_width >= min_x && this_mipmap_height >= min_y);
}

std::optional<ImageData> TextureImage::LoadMipmapFromDisk(size_t level)
{
    std::string path_to_mipmap = modelFolder + "/mipmaps/" + identifierString + "/mipmap_" + std::to_string(level) + ".png";
    if (std::filesystem::exists(path_to_mipmap)) {
        bool _16bit_channel = stbi_is_16_bit(path_to_mipmap.c_str());
        int width = -1;
        int height = -1;
        int components = 0;

        stbi_info(path_to_mipmap.c_str(), &width, &height, &components);
        if (components == 3) components = 4;

        ImageData return_imageData(width, height, components, wrap_S, wrap_T);
        if (_16bit_channel) {
            uint16_t* data = stbi_load_16(path_to_mipmap.c_str(), &width, &height, &components, components);

            std::vector<uint16_t> data_vec;
            std::copy(data, data + width*height*components, std::back_inserter(data_vec));
            stbi_image_free(data);

            return_imageData.SetImage(data_vec);
        } else {
            uint8_t* data = stbi_load(path_to_mipmap.c_str(), &width, &height, &components, components);

            std::vector<uint8_t> data_vec;
            std::copy(data, data + width*height*components, std::back_inserter(data_vec));
            stbi_image_free(data);

            return_imageData.SetImage(data_vec, sRGBifPossible);
        }

        return return_imageData;
    } else {
        return std::nullopt;
    }
}

void TextureImage::SaveMipmaps() const
{
    for(size_t level = 0; level != imagesData.size(); ++level) {
        SaveMipmapToDisk(level);
    }
}

void TextureImage::SaveMipmapToDisk(size_t level) const
{
    std::string path_to_mipmaps_folder = modelFolder + "/mipmaps";
    if (not std::filesystem::exists(path_to_mipmaps_folder))
        std::filesystem::create_directory(path_to_mipmaps_folder);

    std::string path_to_mipmap_texture_folder = path_to_mipmaps_folder + "/" + identifierString;
    if (not std::filesystem::exists(path_to_mipmap_texture_folder))
        std::filesystem::create_directory(path_to_mipmap_texture_folder);

    std::string path_to_this_mipmap = path_to_mipmap_texture_folder + "/mipmap_" + std::to_string(level) + ".png";
    if (not std::filesystem::exists(path_to_this_mipmap)) {
        int width = int(imagesData[level].GetWidth());
        int height = int(imagesData[level].GetHeight());
        int components = int(imagesData[level].GetComponentsCount());

        std::vector<std::byte> data;
        int bytes_per_texel = 0;
        if (saveAs16bit) {
            data = imagesData[level].GetImage16BitPerChannel();
            bytes_per_texel = int(imagesData[level].GetComponentsCount()) * 2;
            stbi_write_png_16(path_to_this_mipmap.c_str(), width, height, components, data.data(), bytes_per_texel * width);
        } else {
            data = imagesData[level].GetImage8BitPerChannel(sRGBifPossible);
            bytes_per_texel = int(imagesData[level].GetComponentsCount());
            stbi_write_png(path_to_this_mipmap.c_str(), width, height, components, data.data(), bytes_per_texel * width);
        }
    }
}

LinearImage::LinearImage(const tinygltf::Image &gltf_image,
                         std::string identifier_string,
                         std::string model_folder,
                         glTFsamplerWrap wrap_S,
                         glTFsamplerWrap wrap_T)
                         : TextureImage(gltf_image,
                                        identifier_string,
                                        model_folder,
                                        wrap_S, wrap_T,
                                        true,
                                        false)
{
}

ImageData LinearImage::CreateMipmap(const ImageData &reference, size_t dimension_factor) const
{
    if (dimension_factor == 1)
        return reference;

    ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                               reference.GetComponentsCount(), reference.GetWrapS(), reference.GetWrapT());

    // Box filter
    for (size_t x = 0; x != mipmap_imageData.GetWidth(); ++x) {
        for (size_t y = 0; y != mipmap_imageData.GetWidth(); ++y) {
            for (size_t comp = 0; comp != mipmap_imageData.GetComponentsCount(); ++comp) {
                float sum_value = 0.f;
                float weight = 0.f;
                for (size_t i = 0; i != dimension_factor; ++i) {
                    for (size_t j = 0; j != dimension_factor; ++j) {
                        sum_value += reference.GetComponent(int(x*dimension_factor + i), int(y*dimension_factor + j),
                                                            comp);
                        weight += 1.f;
                    }
                }

                float value = sum_value / weight;
                mipmap_imageData.SetComponent(x, y, comp, value);
            }
        }
    }

    return mipmap_imageData;
}

NormalImage::NormalImage(const tinygltf::Image &gltf_image,
                         std::string identifier_string,
                         std::string model_folder,
                         glTFsamplerWrap wrap_S,
                         glTFsamplerWrap wrap_T,
                         float in_scale)
                         : TextureImage(gltf_image,
                                        identifier_string,
                                        model_folder,
                                        wrap_S, wrap_T,
                                        false,
                                        true),
                           scale(in_scale)
{
}

ImageData NormalImage::CreateMipmap(const ImageData &reference, size_t dimension_factor) const
{
    if (dimension_factor == 1)
        return reference;

    assert(reference.GetComponentsCount() == 4);
    ImageData mipmap_imageData(reference.GetWidth() / dimension_factor, reference.GetHeight() / dimension_factor,
                               4, reference.GetWrapS(), reference.GetWrapT());

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

            glm::vec3 value = glm::normalize(sum_value);
            value.x /= scale;
            value.y /= scale;
            value = (value + 1.f) / 2.f;

            mipmap_imageData.SetComponent(x, y, 0, value.x);
            mipmap_imageData.SetComponent(x, y, 1, value.y);
            mipmap_imageData.SetComponent(x, y, 2, value.z);
            mipmap_imageData.SetComponent(x, y, 3, 1.f);
        }
    }

    return mipmap_imageData;
}


