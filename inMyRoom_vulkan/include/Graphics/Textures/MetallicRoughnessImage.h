#pragma once

#include "Graphics/Textures/TextureImage.h"

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