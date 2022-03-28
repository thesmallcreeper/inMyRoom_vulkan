#pragma once

#include "Graphics/ImageData.h"
#include "tiny_gltf.h"

float GaussianFilterFactor (float x, float y, float sigma);

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