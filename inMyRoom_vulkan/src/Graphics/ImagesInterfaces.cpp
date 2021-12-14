#include "Graphics/ImagesInterfaces.h"

#include <limits>
#include <cmath>
#include <filesystem>
#include <iostream>

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

std::vector<std::byte> ImageData::GetImage(bool srgb, bool _16bit) const
{
    assert(not (srgb && _16bit));

    size_t buffer_size = width * height * componentsCount * (_16bit ? 2 : 1);

    std::vector<std::byte> buffer(buffer_size);

    if (not _16bit) {
        auto buffer_ptr = reinterpret_cast<uint8_t*>(buffer.data());

        for (size_t i = 0; i != width; ++i) {
            for (size_t j = 0; j != height; ++j) {
                for (size_t k = 0; k != componentsCount; ++k) {
                    float component = GetComponent(i, j, k);

                    uint8_t data = 0;
                    if (srgb) data = FloatToSRGB(component);
                    else data = FloatToR8(component);

                    buffer_ptr[(j * width + i) * componentsCount + k] = data;
                }
            }
        }
    } else {
        auto buffer_ptr = reinterpret_cast<uint16_t*>(buffer.data());

        for (size_t i = 0; i != width; ++i) {
            for (size_t j = 0; j != height; ++j) {
                for (size_t k = 0; k != componentsCount; ++k) {
                    float component = GetComponent(i, j, k);

                    uint16_t data = FloatToR16(component);
                    buffer_ptr[(j * width + i) * componentsCount + k] = data;
                }
            }
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
    value = std::max(value, 0.f);
    value = std::min(value, 1.f);

    float denormalized_value = std::round(value * float(std::numeric_limits<uint8_t>::max()));
    uint8_t quantized_value = uint8_t(denormalized_value);
    return quantized_value;
}

uint16_t ImageData::FloatToR16(float value)
{
    value = std::max(value, 0.f);
    value = std::min(value, 1.f);

    float denormalized_value = std::round(value * float(std::numeric_limits<uint16_t>::max()));
    uint16_t quantized_value = uint16_t(denormalized_value);
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


LinearImage::LinearImage(const tinygltf::Image &gltf_image, std::string identifier_string, std::string model_folder,
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

void LinearImage::RetrieveMipmaps(size_t min_x, size_t min_y)
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

//    // Scale them
//    {
//        assert(reverseScaleFactors.empty());
//        std::vector<float> components_max = GetComponentsMax();
//
//        std::vector<float> normalize_scales;
//        for (float this_max: components_max) {
//            if(this_max == 0.f) this_max = 1.f;
//            normalize_scales.emplace_back(1.f / this_max);
//            reverseScaleFactors.emplace_back(this_max);
//        }
//
//        ScaleComponents(normalize_scales);
//    }

}

std::optional<ImageData> LinearImage::LoadMipmapFromDisk(size_t level)
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

void LinearImage::SaveMipmaps() const
{
    for(size_t level = 0; level != imagesData.size(); ++level) {
        SaveMipmapToDisk(level);
    }
}

void LinearImage::SaveMipmapToDisk(size_t level) const
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
            data = imagesData[level].GetImage(false, true);
            bytes_per_texel = int(imagesData[level].GetComponentsCount()) * 2;
        } else {
            if (sRGBifPossible) data = imagesData[level].GetImage(true, false);
            else data = imagesData[level].GetImage(false, false);

            bytes_per_texel = int(imagesData[level].GetComponentsCount());
        }

        stbi_write_png(path_to_this_mipmap.c_str(), width, height, components, data.data(), bytes_per_texel * width);
    }
}

std::vector<float> LinearImage::GetComponentsMax() const
{
    assert(imagesData.size());

    std::vector<float> components_max = imagesData[0].GetComponentsMax();
    for(size_t i = 1; i != imagesData.size(); ++i) {
        std::vector<float> this_max = imagesData[i].GetComponentsMax();

        assert(this_max.size() == components_max.size());
        for (size_t comp = 0; comp != components_max.size(); ++comp) {
            components_max[comp] = std::max(this_max[comp], components_max[comp]);
        }
    }

    return components_max;
}

void LinearImage::ScaleComponents(const std::vector<float> &scales)
{
    for( auto& this_imageData: imagesData) {
        this_imageData.ScaleComponents(scales);
    }
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

