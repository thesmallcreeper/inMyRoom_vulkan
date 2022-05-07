#pragma once

#include <unordered_map>

#include "glTFenum.h"
#include "vulkan/vulkan.hpp"
#include "shaderc/shaderc.hpp"

inline const std::unordered_map<int, vk::Format> componentsCountToVulkanGammaFormat_map
{
    {1, vk::Format::eR8Srgb},
    {2, vk::Format::eR8G8Srgb},
    {3, vk::Format::eR8G8B8Srgb},
    {4, vk::Format::eR8G8B8A8Srgb}
};

inline const std::unordered_map<int, vk::Format> componentsCountToVulkanLinearFormat_map
{
    {1, vk::Format::eR8Unorm},
    {2, vk::Format::eR8G8Unorm},
    {3, vk::Format::eR8G8B8Unorm},
    {4, vk::Format::eR8G8B8A8Unorm}
};

inline const std::unordered_map<vk::Format, vk::Format> vulkanFormatToCompressedFormat_map
{
    {vk::Format::eR8G8B8Srgb, vk::Format::eBc7SrgbBlock},
    {vk::Format::eR8G8B8A8Srgb, vk::Format::eBc7SrgbBlock},
    {vk::Format::eR8G8B8Unorm, vk::Format::eBc7UnormBlock},
    {vk::Format::eR8G8B8A8Unorm, vk::Format::eBc7UnormBlock}
};

inline const std::unordered_map<glTFsamplerWrap, vk::SamplerAddressMode> glTFsamplerWrapToAddressMode_map
{
    {glTFsamplerWrap::clamp_to_edge, vk::SamplerAddressMode::eClampToEdge},
    {glTFsamplerWrap::mirrored_repeat, vk::SamplerAddressMode::eMirroredRepeat},
    {glTFsamplerWrap::repeat, vk::SamplerAddressMode::eRepeat}
};

inline const std::unordered_map<vk::ShaderStageFlagBits, shaderc_shader_kind> shaderStageToShadercShaderKind_map
{
    {vk::ShaderStageFlagBits::eFragment,                shaderc_fragment_shader},
    {vk::ShaderStageFlagBits::eGeometry,                shaderc_geometry_shader},
    {vk::ShaderStageFlagBits::eTessellationEvaluation,  shaderc_tess_evaluation_shader},
    {vk::ShaderStageFlagBits::eTessellationControl,     shaderc_tess_control_shader},
    {vk::ShaderStageFlagBits::eVertex,                  shaderc_vertex_shader},
    {vk::ShaderStageFlagBits::eCompute,                 shaderc_compute_shader}
};

inline const std::unordered_map<glTFmode, vk::PrimitiveTopology> glTFmodeToPrimitiveTopology_map
{
    {glTFmode::points, vk::PrimitiveTopology::ePointList},
    {glTFmode::line, vk::PrimitiveTopology::eLineList},
    {glTFmode::line_strip, vk::PrimitiveTopology::eLineStrip},
    {glTFmode::triangles, vk::PrimitiveTopology::eTriangleList},
    {glTFmode::triangle_strip, vk::PrimitiveTopology::eTriangleStrip},
    {glTFmode::triangle_fan, vk::PrimitiveTopology::eTriangleFan}
};

inline const std::unordered_map<uint32_t, vk::SampleCountFlagBits> samplesCountToVulkanSampleCountFlag_map
{
        {1, vk::SampleCountFlagBits::e1},
        {2, vk::SampleCountFlagBits::e2},
        {4, vk::SampleCountFlagBits::e4},
        {8, vk::SampleCountFlagBits::e8},
        {16, vk::SampleCountFlagBits::e16}
};