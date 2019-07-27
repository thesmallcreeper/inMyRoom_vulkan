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

class ImagesUsageOfTextures
{
public:
    ImagesUsageOfTextures(tinygltf::Model& in_model);
    ~ImagesUsageOfTextures();

public:
    std::vector<ImageUsage> imagesUsage;
};
