#pragma once

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

class TexturesImagesUsage
{
public:
    TexturesImagesUsage(tinygltf::Model& in_model);
    ~TexturesImagesUsage();

    std::vector<ImageUsage> imagesUsage;
};
