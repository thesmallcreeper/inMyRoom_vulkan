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
    Anvil::ShaderModuleStageEntryPoint* computeShaderModule_ptr = nullptr;

    bool operator==(const ShadersSet& rhs)
    {
        bool isEqual = (fragmentShaderModule_ptr == rhs.fragmentShaderModule_ptr) &&
            (geometryShaderModule_ptr == rhs.geometryShaderModule_ptr) &&
            (tessControlShaderModule_ptr == rhs.tessControlShaderModule_ptr) &&
            (tessEvaluationShaderModule_ptr == rhs.tessEvaluationShaderModule_ptr) &&
            (vertexShaderModule_ptr == rhs.vertexShaderModule_ptr) &&
            (computeShaderModule_ptr == rhs.computeShaderModule_ptr);

        return isEqual;
    }
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
};

struct ShadersSetsFamilySourceStrings
{
    std::string fragmentShaderSourceString;
    std::string geometryShaderSourceString;
    std::string tessControlShaderSourceString;
    std::string tessEvaluationShaderSourceString;
    std::string vertexShaderSourceString;
    std::string computeShaderSourceString;
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
            else
                return false;

            if (lhs.definitionValuePairs.size() == rhs.definitionValuePairs.size() && isEqual)
                for (size_t i = 0; i < lhs.definitionValuePairs.size() && isEqual; i++)
                {
                    isEqual &= lhs.definitionValuePairs[i] == rhs.definitionValuePairs[i];
                }
            else
                return false;

            return isEqual;
        }
    };
}

class ShadersSetsFamiliesCache
{
public: //functions
    ShadersSetsFamiliesCache(Anvil::BaseDevice* const in_device_ptr);

    void AddShadersSetsFamily(ShadersSetsFamilyInitInfo in_shadersSetsFamilyInitInfos);

    ShadersSet GetShadersSet(ShadersSpecs in_shaderSpecs);

private: //functions
    ShadersSet CreateShadersSet(ShadersSpecs in_shaderSpecs);

private: // data
    Anvil::BaseDevice* const device_ptr;

    std::vector<ShadersSet> shadersSets;

    std::vector<std::unique_ptr<Anvil::ShaderModuleStageEntryPoint>> shaderModulesStageEntryPoints_uptrs;

    std::unordered_map<std::string, ShadersSetsFamilySourceStrings> shadersSetFamilyNameToShadersSetFamilySourceStrings_umap;
    std::unordered_map<ShadersSpecs, size_t> shaderSpecsToShadersSetIndex_umap;
};
