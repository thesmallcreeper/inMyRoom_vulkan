#pragma once

#include <string>
#include <optional>

#include "tiny_gltf.h"
#include "glTFenum.h"

class ImageData
{
public:
    ImageData(size_t width, size_t height, size_t components,
              glTFsamplerWrap wrap_S, glTFsamplerWrap wrap_T);
    ImageData(const ImageData& image_data, std::vector<bool> channel_select);

    void SetImage(const std::vector<uint8_t>& data, bool is_srgb);
    void SetImage(const std::vector<uint16_t>& data);
    std::vector<std::byte> GetImage8BitPerChannel(bool srgb) const;
    std::vector<std::byte> GetImage16BitPerChannel() const;
    std::vector<std::byte> GetImageA2R10G10B10() const;

    size_t GetWidth() const {return width;}
    size_t GetHeight() const {return height;}
    size_t GetComponentsCount() const {return componentsCount;}
    glTFsamplerWrap GetWrapS() const {return wrap_S;}
    glTFsamplerWrap GetWrapT() const {return wrap_T;}

    float GetComponent(int x, int y, size_t component) const;
    void SetComponent(size_t x, size_t y, size_t component, float value);

    std::vector<float> GetComponentsMax() const;
    std::vector<float> GetComponentsMin() const;
    void ScaleComponents(const std::vector<float>& scales);
    void BiasComponents(const std::vector<float>& bias);

    bool operator> (const ImageData& rhs) const {return width*height > rhs.width*rhs.height;}

private:
    static float SRGBtoFloat(uint8_t value);
    static float R8toFloat(uint8_t value);
    static float R16toFloat(uint16_t value);

    static uint8_t FloatToSRGB(float value);
    static uint8_t FloatToR8(float value);
    static uint16_t FloatToR16(float value);
    static uint32_t FloatToRx(float value, uint32_t max);

    static size_t WrapAddress(glTFsamplerWrap wrap, size_t size, int address);

private:
    size_t width;
    size_t height;
    size_t componentsCount;
    glTFsamplerWrap wrap_S;
    glTFsamplerWrap wrap_T;

    std::vector<float> floatBuffer;
};

class TextureImage
{
public:
    TextureImage(const tinygltf::Image* gltf_image_ptr,
                 std::string identifier_string,
                 std::string model_folder,
                 glTFsamplerWrap wrap_S,
                 glTFsamplerWrap wrap_T,
                 bool sRGB,
                 bool saveAs16bit);

    void RetrieveMipmaps(size_t min_x, size_t min_y);
    void SaveMipmaps() const;

    const std::vector<ImageData>& GetMipmaps() const {assert(not imagesData.empty()); return imagesData;}

protected:
    virtual ImageData CreateMipmap(const ImageData& reference, size_t dimension_factor) = 0;

    std::optional<ImageData> LoadMipmapFromDisk(size_t level);
    void SaveMipmapToDisk(size_t level) const;

protected:
    std::vector<ImageData> imagesData;

    const tinygltf::Image* glTFimage_ptr;
    std::string identifierString;
    std::string modelFolder;
    glTFsamplerWrap wrap_S;
    glTFsamplerWrap wrap_T;
    bool sRGBifPossible;
    bool saveAs16bit;
};

class ColorImage
    : public TextureImage
{
public:
    ColorImage(const tinygltf::Image* gltf_image_ptr,
               std::string identifier_string,
               std::string model_folder,
               glTFsamplerWrap wrap_S,
               glTFsamplerWrap wrap_T);

private:
    ImageData CreateMipmap(const ImageData& reference, size_t dimension_factor) override;
};

class NormalImage
    : public TextureImage
{
public:
    NormalImage(const tinygltf::Image* gltf_image_ptr,
                std::string identifier_string,
                std::string model_folder,
                glTFsamplerWrap wrap_S,
                glTFsamplerWrap wrap_T,
                float scale);

    const std::unordered_map<uint32_t, ImageData>& GetWidthToLengthsDataUmap() const {return widthToLengthsData;}

private:
    ImageData CreateMipmap(const ImageData& reference, size_t dimension_factor) override;

private:
    float scale = 1.f;
    std::unordered_map<uint32_t, ImageData> widthToLengthsData;
};

class MetallicRoughnessImage
    : public TextureImage
{
public:
    MetallicRoughnessImage(const tinygltf::Image* gltf_image_ptr,
                           std::string identifier_string,
                           std::string model_folder,
                           glTFsamplerWrap wrap_S,
                           glTFsamplerWrap wrap_T,
                           float metallic_factor,
                           float roughness_factor,
                           const std::unordered_map<uint32_t, ImageData>& widthToLengthsData);

private:
    ImageData CreateMipmap(const ImageData& reference, size_t dimension_factor) override;

private:
    float metallic_factor;
    float roughness_factor;

    const std::unordered_map<uint32_t, ImageData>& widthToLengthsData;
};