#pragma once

#ifdef __GNUC__
#if __GNUC__ < 8
#define FILESYSTEM_IS_EXPERIMENTAL
#else
#endif
#endif

#include "tiny_gltf.h"

#include <utility>
#include <cassert>

#ifdef FILESYSTEM_IS_EXPERIMENTAL
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "glTFenum.h"

#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/sampler_create_info.h"
#include "wrappers/device.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/sampler.h"

#include "Compressonator.h"



struct TextureInfo
{
    size_t imageIndex;
    size_t samplerIndex;
};

class MaterialsTextures
{
public:
    MaterialsTextures(tinygltf::Model& in_model, const std::string& in_imagesFolder, Anvil::BaseDevice* in_device_ptr);
    ~MaterialsTextures();

    std::vector<TextureInfo> textures;

    std::vector<Anvil::ImageUniquePtr> images;
    std::vector<Anvil::ImageViewUniquePtr> imagesViews;
    std::vector<Anvil::SamplerUniquePtr> samplers;

private:
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
};
