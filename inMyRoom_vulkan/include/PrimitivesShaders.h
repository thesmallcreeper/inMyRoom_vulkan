#pragma once

#include <vector>
#include <string>
#include <utility>

#include <unordered_map>
#include "hash_combine.h"

#include "misc/glsl_to_spirv.h"
#include "wrappers/shader_module.h"
#include "wrappers/device.h"

struct ShadersSet
{
    Anvil::ShaderModuleStageEntryPoint* fragmentShaderModule_ptr = nullptr;
    Anvil::ShaderModuleStageEntryPoint* geometryShaderModule_ptr = nullptr;
    Anvil::ShaderModuleStageEntryPoint* tessControlShaderModule_ptr = nullptr;
    Anvil::ShaderModuleStageEntryPoint* tessEvaluationShaderModule_ptr = nullptr;
    Anvil::ShaderModuleStageEntryPoint* vertexShaderModule_ptr = nullptr;
};

inline bool operator==(const ShadersSet& lhs, const ShadersSet& rhs)
{
    bool isEqual = (lhs.fragmentShaderModule_ptr == rhs.fragmentShaderModule_ptr) &&
        (lhs.geometryShaderModule_ptr == rhs.geometryShaderModule_ptr) &&
        (lhs.tessControlShaderModule_ptr == rhs.tessControlShaderModule_ptr) &&
        (lhs.tessEvaluationShaderModule_ptr == rhs.tessEvaluationShaderModule_ptr) &&
        (lhs.vertexShaderModule_ptr == rhs.vertexShaderModule_ptr);

    return isEqual;
}

struct ShaderSetFamilyInitInfo
{
    std::string shadersSetFamilyName;
    std::string fragmentShaderSourceFilename;
    std::string geometryShaderSourceFilename;
    std::string tessControlShaderSourceFilename;
    std::string tessEvaluationShaderSourceFilename;
    std::string vertexShaderSourceFilename;
};

struct ShaderSetFamilySourceStrings
{
    std::string fragmentShaderSourceString;
    std::string geometryShaderSourceString;
    std::string tessControlShaderSourceString;
    std::string tessEvaluationShaderSourceString;
    std::string vertexShaderSourceString;
};

struct ShadersSpecs
{
    std::string shadersSetFamilyName;
    std::vector<std::string> emptyDefinition;
    std::vector<std::pair<std::string, int32_t>> definitionValuePairs;
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

            for (const auto& this_emptyDefinition : in_pipelineSpecs.emptyDefinition)
            {
                hash_combine(result, this_emptyDefinition);
            }

            for (const auto& this_definitionValuePair : in_pipelineSpecs.definitionValuePairs)
            {
                hash_combine(result, this_definitionValuePair.first);
                hash_combine(result, this_definitionValuePair.second);
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

            if (lhs.emptyDefinition.size() == rhs.emptyDefinition.size() && isEqual)
                for (size_t i = 0; i < lhs.emptyDefinition.size() && isEqual; i++)
                {
                    isEqual &= lhs.emptyDefinition[i] == rhs.emptyDefinition[i];
                }

            if (lhs.definitionValuePairs.size() == rhs.definitionValuePairs.size() && isEqual)
                for (size_t i = 0; i < lhs.definitionValuePairs.size() && isEqual; i++)
                {
                    isEqual &= lhs.definitionValuePairs[i] == rhs.definitionValuePairs[i];
                }

            return isEqual;
        }
    };
}

class PrimitivesShaders
{
public:
    PrimitivesShaders(std::vector<ShaderSetFamilyInitInfo> in_shadersSetFamilyInitInfos,
                      Anvil::BaseDevice* in_device_ptr);
    ~PrimitivesShaders();

    size_t getShaderSetIndex(ShadersSpecs in_shaderSpecs);
    std::vector<ShadersSet> shadersSets;

private:
    ShadersSet createShadersSet(ShadersSpecs in_shaderSpecs);

    std::vector<std::unique_ptr<Anvil::ShaderModuleStageEntryPoint>> shaderModulesStageEntryPoints_ptrs;

    std::unordered_map<std::string, ShaderSetFamilySourceStrings>
    shadersSetFamilyNameToShadersSetFamilySourceStrings_umap;
    std::unordered_map<ShadersSpecs, size_t> shaderSpecsToShaderSetIndex_umap;

    Anvil::BaseDevice* device_ptr;
};
