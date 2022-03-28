#pragma once

#include "Graphics/Textures/TextureImage.h"

class ColorImage
        : public TextureImage
{
public:
    ColorImage(const tinygltf::Image* gltf_image_ptr,
               std::string identifier_string,
               std::string model_folder,
               glTFsamplerWrap wrap_S,
               glTFsamplerWrap wrap_T,
               float filter_sigma = 1.f);

private:
    ImageData CreateMipmap(const ImageData& reference, size_t dimension_factor) override;

    const float filterSigma;
};