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
public:  //functions
    void registImagesOfModel(tinygltf::Model& in_model);
    ImageUsage getImageUsage(tinygltf::Image& in_image) const;

private: //functions
    void registImage(tinygltf::Image& in_image, ImageUsage in_imageUsage);
    static std::string ImageInfoToString(tinygltf::Image& in_image);
private: //data
    std::unordered_map<std::string, ImageUsage> imagesUsage_umap;
};
