#pragma once

#include <map>
#include <unordered_map>

#include "glTFenum.h"
#include "CMP_CompressonatorLib/Compressonator.h"
#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/sampler_create_info.h"

const std::map<int, Anvil::Format> componentsCountToVulkanGammaFormat_map
{
    {1, Anvil::Format::R8_SRGB},
    {2, Anvil::Format::R8G8_SRGB},
    {3, Anvil::Format::R8G8B8_SRGB},
    {4, Anvil::Format::R8G8B8A8_SRGB}
};

const std::map<int, Anvil::Format> componentsCountToVulkanLinearFormat_map
{
    {1, Anvil::Format::R8_UNORM},
    {2, Anvil::Format::R8G8_UNORM},
    {3, Anvil::Format::R8G8B8_UNORM},
    {4, Anvil::Format::R8G8B8A8_UNORM}
};

const std::map<Anvil::Format, Anvil::Format> vulkanFormatTo16BitPerChannel_map
{
    {Anvil::Format::R8_SRGB, Anvil::Format::R16_UNORM},
    {Anvil::Format::R8G8_SRGB, Anvil::Format::R16G16_UNORM},
    {Anvil::Format::R8G8B8A8_SRGB, Anvil::Format::R16G16B16A16_UNORM},
    {Anvil::Format::R8_UNORM, Anvil::Format::R16_UNORM},
    {Anvil::Format::R8G8_UNORM, Anvil::Format::R16G16_UNORM},
    {Anvil::Format::R8G8B8A8_UNORM, Anvil::Format::R16G16B16A16_UNORM}
};

const std::map<Anvil::Format, Anvil::Format> vulkanFormatToCompressedFormat_map
{
    {Anvil::Format::R8G8B8_SRGB, Anvil::Format::BC7_SRGB_BLOCK},
    {Anvil::Format::R8G8B8A8_SRGB, Anvil::Format::BC7_SRGB_BLOCK},
    {Anvil::Format::R8G8B8_UNORM, Anvil::Format::BC7_UNORM_BLOCK},
    {Anvil::Format::R8G8B8A8_UNORM, Anvil::Format::BC7_UNORM_BLOCK}
};

const std::map<Anvil::Format, CMP_FORMAT> vulkanFormatToCompressonatorFormat_map
{
    {Anvil::Format::R8_SRGB, CMP_FORMAT_R_8},
    {Anvil::Format::R8G8_SRGB, CMP_FORMAT_RG_8},
    {Anvil::Format::R8G8B8_UNORM, CMP_FORMAT_RGB_888},
    {Anvil::Format::R8G8B8A8_UNORM, CMP_FORMAT_RGBA_8888},
    {Anvil::Format::R8G8B8_SRGB, CMP_FORMAT_RGB_888},
    {Anvil::Format::R8G8B8A8_SRGB, CMP_FORMAT_RGBA_8888},
    {Anvil::Format::BC7_SRGB_BLOCK, CMP_FORMAT_BC7},
    {Anvil::Format::BC7_UNORM_BLOCK, CMP_FORMAT_BC7}
};

const std::map<glTFsamplerMagFilter, Anvil::Filter> glTFsamplerMagFilterToFilter_map
{
    {glTFsamplerMagFilter::nearest, Anvil::Filter::NEAREST},
    {glTFsamplerMagFilter::linear, Anvil::Filter::LINEAR}
};

const Anvil::SamplerMipmapMode defaultMipmapMode = Anvil::SamplerMipmapMode::LINEAR;
const std::map<glTFsamplerMinFilter, std::pair<Anvil::Filter, Anvil::SamplerMipmapMode>> glTFsamplerMinFilterToFilterAndMipmapMode_map
{
    {glTFsamplerMinFilter::nearest, {Anvil::Filter::NEAREST, defaultMipmapMode}},
    {glTFsamplerMinFilter::linear, {Anvil::Filter::LINEAR, defaultMipmapMode}},
    {glTFsamplerMinFilter::nearest_mipmap_nearest, {Anvil::Filter::NEAREST, Anvil::SamplerMipmapMode::NEAREST}},
    {glTFsamplerMinFilter::linear_mipmap_nearest, {Anvil::Filter::LINEAR, Anvil::SamplerMipmapMode::NEAREST}},
    {glTFsamplerMinFilter::nearest_mipmap_linear, {Anvil::Filter::NEAREST, Anvil::SamplerMipmapMode::LINEAR}},
    {glTFsamplerMinFilter::linear_mipmap_linear, {Anvil::Filter::LINEAR, Anvil::SamplerMipmapMode::LINEAR}},
};

const std::map<glTFsamplerWrap, Anvil::SamplerAddressMode> glTFsamplerWrapToAddressMode_map
{
    {glTFsamplerWrap::clamp_to_edge, Anvil::SamplerAddressMode::CLAMP_TO_EDGE},
    {glTFsamplerWrap::mirrored_repeat, Anvil::SamplerAddressMode::MIRRORED_REPEAT},
    {glTFsamplerWrap::repeat, Anvil::SamplerAddressMode::REPEAT}
};
