#pragma once

#include <unordered_map>
#include <string>

#include "tiny_gltf.h"

enum class ImageUsage
{
    undefined = 0,

    baseColor,
    metallic,
    roughness,
    normal,
    occlusion,
    emissive
};

class ImagesUsageOfTextures
{
public:  // functions
    void AddImagesUsageOfModel(const tinygltf::Model& in_model);
    ImageUsage GetImageUsage(const tinygltf::Image& in_image) const;

private: // functions
    void RegistImage(const tinygltf::Image& in_image, ImageUsage in_imageUsage);

private: // data
    std::unordered_map<tinygltf::Image*, ImageUsage> imagesUsage_umap;
};
