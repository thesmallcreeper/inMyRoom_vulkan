#pragma once

#include <unordered_map>

#include "tiny_gltf.h"
#include "glTFenum.h"

enum class ImageMap
{
    undefined = 0,

    baseColor           = 1 << 0,
    metallicRoughness   = 1 << 1,
    normal              = 1 << 2,
    occlusion           = 1 << 3,
    emissive            = 1 << 4,
};
inline ImageMap operator|(ImageMap a, ImageMap b)
{
    return static_cast<ImageMap>(static_cast<int>(a) | static_cast<int>(b));
}

struct ImageAbout
{
    ImageMap map = ImageMap::undefined;
    glTFsamplerWrap wrapS = glTFsamplerWrap::mirrored_repeat;
    glTFsamplerWrap wrapT = glTFsamplerWrap::mirrored_repeat;
    const tinygltf::Image* sibling_baseColor_image_ptr = nullptr;
    const tinygltf::Image* sibling_metallicRoughness_image_ptr = nullptr;
    const tinygltf::Image* sibling_normal_image_ptr = nullptr;
    const tinygltf::Image* sibling_occlusion_image_ptr = nullptr;
    const tinygltf::Image* sibling_emissive_image_ptr = nullptr;
    const tinygltf::Material* material_example_ptr = nullptr;
};


// It isn't flowless ,because materials can find loopholes in it, but for a normal scene it is fine :)
// Regist how the first material to use a image will use it.
// Could go full length to make my shit flawless, sthing that would be pita.
// anyway...

class ImagesAboutOfTextures 
{
public:  // functions
    void AddImagesUsageOfModel(const tinygltf::Model& in_model);
    ImageAbout GetImageAbout(const tinygltf::Image& in_image) const;

private: // functions
    void RegistImage(ImageAbout in_imageUsage);

private: // data
    std::unordered_map<tinygltf::Image*, ImageAbout> imagesUsage_umap;
};
