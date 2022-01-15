#pragma once

#include <vector>
#include <string>
#include <unordered_set>
#include <utility>

#include <unordered_map>
#include "hash_combine.h"

#include "vulkan/vulkan.hpp"
#include "shaderc/shaderc.hpp"

#include "Graphics/VulkanInit.h"

struct ShadersSet
{
    vk::ShaderModule fragmentShaderModule;
    vk::ShaderModule geometryShaderModule;
    vk::ShaderModule tessControlShaderModule;
    vk::ShaderModule tessEvaluationShaderModule;
    vk::ShaderModule vertexShaderModule;
    vk::ShaderModule computeShaderModule;

    bool abortedDueToDefinition = false;
};

struct ShadersSetsFamilyInitInfo
{
    std::string shadersSetFamilyName;

    std::string fragmentShaderSourceFilename;
    std::string geometryShaderSourceFilename;
    std::string tessControlShaderSourceFilename;
    std::string tessEvaluationShaderSourceFilename;
    std::string vertexShaderSourceFilename;
    std::string computeShaderSourceFilename;

    std::unordered_set<std::string> abortShaderIfDefinitionFound;
};

struct ShadersSetsFamilySourceStrings
{
    std::string fragmentShaderSourceString;
    std::string geometryShaderSourceString;
    std::string tessControlShaderSourceString;
    std::string tessEvaluationShaderSourceString;
    std::string vertexShaderSourceString;
    std::string computeShaderSourceString;

    std::unordered_set<std::string> abortShaderIfDefinitionFound;
};

struct ShadersSpecs
{
    std::string shadersSetFamilyName;
    std::vector<std::pair<std::string, std::string>> definitionStringPairs;
};

namespace std
{
    template <>
    struct hash<ShadersSpecs>
    {
        std::size_t operator()(const ShadersSpecs& in_pipelineSpecs) const
        {
            std::size_t result = 0;
            hash_combine(result, in_pipelineSpecs.shadersSetFamilyName);

            for (const auto& this_definitionStringPair : in_pipelineSpecs.definitionStringPairs)
            {
                hash_combine(result, this_definitionStringPair.first);
                hash_combine(result, this_definitionStringPair.second);
            }

            return result;
        }
    };

    template <>
    struct equal_to<ShadersSpecs>
    {
        bool operator()(const ShadersSpecs& lhs, const ShadersSpecs& rhs) const
        {
            bool isEqual = lhs.shadersSetFamilyName == rhs.shadersSetFamilyName;

            if (lhs.definitionStringPairs.size() == rhs.definitionStringPairs.size() && isEqual)
                for (size_t i = 0; i < lhs.definitionStringPairs.size() && isEqual; i++)
                {
                    isEqual &= lhs.definitionStringPairs[i] == rhs.definitionStringPairs[i];
                }
            else
                return false;

            return isEqual;
        }
    };
}

class ShadersSetsFamiliesCache
{
public:
    ShadersSetsFamiliesCache(vk::Device device,
                             VendorID vendorId,
                             std::string shaders_folder);
    ~ShadersSetsFamiliesCache();
    ShadersSetsFamiliesCache (const ShadersSetsFamiliesCache&) = delete;
    ShadersSetsFamiliesCache& operator= (const ShadersSetsFamiliesCache&) = delete;

    void AddShadersSetsFamily(const ShadersSetsFamilyInitInfo& shadersSetsFamilyInitInfos);
    ShadersSet GetShadersSet(ShadersSpecs shaderSpecs);

private:
    ShadersSet CreateShadersSet(const ShadersSpecs& shaderSpecs);

    std::vector<uint32_t> CompileGLSLtoSPIRv(const std::string& shader_source,
                                             const std::string& family_name,
                                             const std::vector<std::pair<std::string, std::string>>& definition_pairs,
                                             vk::ShaderStageFlagBits shader_stage);

    vk::ShaderModule GetShaderModule(std::vector<uint32_t>&& spirv_binary,
                                     const std::string& message_on_fail);

private:
    const vk::Device device;
    const VendorID vendorID;
    const std::string shadersFolder;

    std::unordered_map<std::string, ShadersSetsFamilySourceStrings> shadersSetFamilyNameToShadersSetFamilySourceStrings_umap;
    std::unordered_map<ShadersSpecs, ShadersSet> shaderSpecsToShadersSet_umap;
    std::unordered_map<std::vector<uint32_t>, vk::ShaderModule> spirvToShaderModule_umap;
};
