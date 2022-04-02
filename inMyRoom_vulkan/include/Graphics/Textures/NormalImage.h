#pragma once

#include "Graphics/Textures/TextureImage.h"

class NormalImage
        : public TextureImage
{
public:
    NormalImage(const tinygltf::Image* gltf_image_ptr,
                std::string identifier_string,
                std::string model_folder,
                glTFsamplerWrap wrap_S,
                glTFsamplerWrap wrap_T,
                float scale,
                float filter_sigma = 1.f);

    const std::unordered_map<uint32_t, ImageData>& GetWidthToLengthsDataUmap() const;

private:
    ImageData CreateMipmap(const ImageData& reference, size_t dimension_factor) override;

private:
    float scale = 1.f;
    std::unordered_map<uint32_t, ImageData> widthToLengthsData;

    const float filterSigma;
};