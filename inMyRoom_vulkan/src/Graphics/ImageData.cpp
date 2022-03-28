#include "Graphics/ImageData.h"

#include <limits>
#include <cmath>
#include <cassert>

ImageData::ImageData(size_t in_width, size_t in_height, size_t in_components,
                     glTFsamplerWrap in_wrap_S, glTFsamplerWrap in_wrap_T)
    : width(in_width),
      height(in_height),
      componentsCount(in_components),
      wrap_S(in_wrap_S),
      wrap_T(in_wrap_T)
{
    floatBuffer.resize(width * height * componentsCount, 0.f);
}

ImageData::ImageData(const ImageData &image_data, std::vector<bool> channel_select)
    : width(image_data.width),
      height(image_data.height),
      wrap_S(image_data.wrap_S),
      wrap_T(image_data.wrap_T)
{
    assert(image_data.componentsCount == channel_select.size());

    componentsCount = 0;
    for(bool channel_bool: channel_select) {
        if (channel_bool) ++componentsCount;
    }

    floatBuffer.resize(width * height * componentsCount);

    for (size_t i = 0; i != width * height; ++i) {
        size_t comp = 0;
        for (size_t j = 0; j != channel_select.size(); ++j) {
            if (channel_select[j]) {
                floatBuffer[i * componentsCount + comp] = image_data.floatBuffer[i * image_data.componentsCount + j];
                ++comp;
            }
        }
    }
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
                if (componentsCount == 3) data_a = 0x0003;
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
    if(wrap == glTFsamplerWrap::repeat) {
        return_address = address % int(size);
        if (return_address < 0) return_address = int(size) + return_address;
    } else if (wrap == glTFsamplerWrap::clamp_to_edge) {
        address = std::min(address, int(size - 1));
        address = std::max(address, 0);
        return_address = address;
    } else if (wrap == glTFsamplerWrap::mirrored_repeat) {
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
