#pragma once

#ifdef __GNUC__
#if __GNUC__ < 8
#define FILESYSTEM_IS_EXPERIMENTAL
#else
#endif
#endif

#include <utility>
#include <cassert>
#include <unordered_map>
#include "hash_combine.h"
#ifdef FILESYSTEM_IS_EXPERIMENTAL
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/sampler_create_info.h"
#include "wrappers/device.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/sampler.h"

#include "tiny_gltf.h"
#include "glTFenum.h"
#include "Compressonator.h"

#include "ImagesAboutOfTextures.h"

struct SamplerSpecs
{
    glTFsamplerMagFilter magFilter;
    glTFsamplerMinFilter minFilter;
    glTFsamplerWrap wrapS;
    glTFsamplerWrap wrapT;
};

namespace std
{
    template <>
    struct hash<SamplerSpecs>
    {
        std::size_t operator()(const SamplerSpecs& in_samplerSpecs) const
        {
            std::size_t result = 0;
            hash_combine(result, in_samplerSpecs.magFilter);
            hash_combine(result, in_samplerSpecs.minFilter);
            hash_combine(result, in_samplerSpecs.wrapS);
            hash_combine(result, in_samplerSpecs.wrapT);
            return result;
        }
    };

    template <>
    struct equal_to<SamplerSpecs>
    {
        bool operator()(const SamplerSpecs& lhs, const SamplerSpecs& rhs) const
        {
            bool isEqual = (lhs.magFilter == rhs.magFilter) &&
                           (lhs.minFilter == rhs.minFilter) &&
                           (lhs.wrapS == rhs.wrapS) &&
                           (lhs.wrapT == rhs.wrapT);

            return isEqual;
        }
    };
}

struct TextureInfo
{
    size_t imageIndex;
    size_t samplerIndex;
};

class TexturesOfMaterials
{
public: // functions
    TexturesOfMaterials(bool use_mipmaps,
                       ImagesAboutOfTextures* in_imagesAboutOfTextures_ptr,
                       Anvil::BaseDevice* const in_device_ptr);
    ~TexturesOfMaterials();

    void AddTexturesOfModel(const tinygltf::Model& in_model, const std::string& in_imagesFolder);
    size_t GetTextureIndexOffsetOfModel(const tinygltf::Model& in_model);

    Anvil::ImageView* GetImageView(size_t in_texture_index);
    Anvil::Sampler* GetSampler(size_t in_texture_index);
     
private: // functions

    size_t GetSamplerIndex(SamplerSpecs in_samplerSpecs);

private: // data
    const bool useMipmaps;

    size_t imagesSoFar;
    size_t texturesSoFar;

    std::vector<TextureInfo> texturesInfos;

    std::vector<Anvil::ImageUniquePtr> images_uptrs;
    std::vector<Anvil::ImageViewUniquePtr> imagesViews_upts;
    std::vector<Anvil::SamplerUniquePtr> samplers_uptrs;

    std::unordered_map<SamplerSpecs, size_t> samplerSpecsToSamplerIndex_umap;

    std::unordered_map<tinygltf::Model*, size_t> modelToTextureIndexOffset_umap;

    const Anvil::Format image_preferred_format = Anvil::Format::BC7_SRGB_BLOCK;

    const Anvil::SamplerMipmapMode defaultMipmapMode = Anvil::SamplerMipmapMode::LINEAR;

    std::map<int, Anvil::Format> componentsCountToVulkanFormat_map
    {
        {1, Anvil::Format::R8_SRGB},
        {2, Anvil::Format::R8G8_SRGB},
        {3, Anvil::Format::R8G8B8_SRGB},
        {4, Anvil::Format::R8G8B8A8_SRGB}
    };

    std::map<Anvil::Format, CMP_FORMAT> vulkanFormatToCompressonatorFormat_map
    {
        {Anvil::Format::R8_SRGB, CMP_FORMAT_R_8},
        {Anvil::Format::R8G8_SRGB, CMP_FORMAT_RG_8},
        {Anvil::Format::R8G8B8_SRGB, CMP_FORMAT_RGB_888},
        {Anvil::Format::R8G8B8A8_SRGB, CMP_FORMAT_RGBA_8888},
        {Anvil::Format::BC7_SRGB_BLOCK, CMP_FORMAT_BC7},
        {Anvil::Format::BC3_SRGB_BLOCK, CMP_FORMAT_BC3}
    };

    std::map<glTFsamplerMagFilter, Anvil::Filter> glTFsamplerMagFilterToFilter_map
    {
        {glTFsamplerMagFilter::nearest, Anvil::Filter::NEAREST},
        {glTFsamplerMagFilter::linear, Anvil::Filter::LINEAR}
    };

    std::map<glTFsamplerMinFilter, std::pair<Anvil::Filter, Anvil::SamplerMipmapMode>> glTFsamplerMinFilterToFilterAndMipmapMode_map
    {
        {glTFsamplerMinFilter::nearest, {Anvil::Filter::NEAREST, defaultMipmapMode}},
        {glTFsamplerMinFilter::linear, {Anvil::Filter::LINEAR, defaultMipmapMode}},
        {glTFsamplerMinFilter::nearest_mipmap_nearest, {Anvil::Filter::NEAREST, Anvil::SamplerMipmapMode::NEAREST}},
        {glTFsamplerMinFilter::linear_mipmap_nearest, {Anvil::Filter::LINEAR, Anvil::SamplerMipmapMode::NEAREST}},
        {glTFsamplerMinFilter::nearest_mipmap_linear, {Anvil::Filter::NEAREST, Anvil::SamplerMipmapMode::LINEAR}},
        {glTFsamplerMinFilter::linear_mipmap_linear, {Anvil::Filter::LINEAR, Anvil::SamplerMipmapMode::LINEAR}},
    };

    std::map<glTFsamplerWrap, Anvil::SamplerAddressMode> glTFsamplerWrapToAddressMode_map
    {
        {glTFsamplerWrap::clamp_to_edge, Anvil::SamplerAddressMode::CLAMP_TO_EDGE},
        {glTFsamplerWrap::mirrored_repeat, Anvil::SamplerAddressMode::MIRRORED_REPEAT},
        {glTFsamplerWrap::repeat, Anvil::SamplerAddressMode::REPEAT}
    };

    ImagesAboutOfTextures* imagesAboutOfTextures_ptr;
    Anvil::BaseDevice* const device_ptr;
};
