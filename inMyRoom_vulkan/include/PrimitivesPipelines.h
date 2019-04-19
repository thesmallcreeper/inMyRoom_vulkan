#pragma once
#include <cassert>

#include <unordered_map>
#include "hash_combine.h"

#include "misc/base_pipeline_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "misc/descriptor_set_create_info.h"
#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/shader_module.h"
#include "wrappers/graphics_pipeline_manager.h"

#include "glTFenum.h"

#include "PrimitivesShaders.h"

struct PipelineSpecs
{
    glTFmode drawMode = static_cast<glTFmode>(-1);
    glTFcomponentType indexComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType positionComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType normalComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType tangentComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType texcoord0ComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType texcoord1ComponentType = static_cast<glTFcomponentType>(-1);
    std::vector<const Anvil::DescriptorSetCreateInfo*> descriptorSetsCreateInfo_ptrs;
    ShadersSet pipelineShaders;
    Anvil::RenderPass* renderpass_ptr;
    Anvil::SubPassID subpassID;
};

namespace std
{
    template <>
    struct hash<PipelineSpecs>
    {
        std::size_t operator()(const PipelineSpecs& in_pipelineSpecs) const
        {
            std::size_t result = 0;
            hash_combine(result, in_pipelineSpecs.drawMode);
            hash_combine(result, in_pipelineSpecs.indexComponentType);
            hash_combine(result, in_pipelineSpecs.positionComponentType);
            hash_combine(result, in_pipelineSpecs.normalComponentType);
            hash_combine(result, in_pipelineSpecs.tangentComponentType);
            hash_combine(result, in_pipelineSpecs.texcoord0ComponentType);
            hash_combine(result, in_pipelineSpecs.texcoord1ComponentType);
            hash_combine(result, in_pipelineSpecs.renderpass_ptr);
            hash_combine(result, in_pipelineSpecs.subpassID);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.fragmentShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.geometryShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.tessControlShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.tessEvaluationShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.vertexShaderModule_ptr);

            //for(const auto& this_descriptorSetCreateInfo_ptr : in_pipelineSpecs.descriptorSetsCreateInfo_ptrs)
            //{
            //    hash_combine(result, *this_descriptorSetCreateInfo_ptr);
            //}

            return result;
        }
    };

    template <>
    struct equal_to<PipelineSpecs>
    {
        bool operator()(const PipelineSpecs& lhs, const PipelineSpecs& rhs) const
        {
            bool isEqual =  (lhs.drawMode == rhs.drawMode) &&
                            (lhs.indexComponentType == rhs.indexComponentType) &&
                            (lhs.positionComponentType == rhs.positionComponentType) &&
                            (lhs.normalComponentType == rhs.normalComponentType) &&
                            (lhs.tangentComponentType == rhs.tangentComponentType) &&
                            (lhs.texcoord0ComponentType == rhs.texcoord0ComponentType) &&
                            (lhs.texcoord1ComponentType == rhs.texcoord1ComponentType) &&
                            (lhs.renderpass_ptr == rhs.renderpass_ptr) &&
                            (lhs.subpassID == rhs.subpassID) &&
                            (lhs.pipelineShaders.fragmentShaderModule_ptr == rhs.pipelineShaders.fragmentShaderModule_ptr) &&
                            (lhs.pipelineShaders.geometryShaderModule_ptr == rhs.pipelineShaders.geometryShaderModule_ptr) &&
                            (lhs.pipelineShaders.tessControlShaderModule_ptr == rhs.pipelineShaders.tessControlShaderModule_ptr) &&
                            (lhs.pipelineShaders.tessEvaluationShaderModule_ptr == rhs.pipelineShaders.tessEvaluationShaderModule_ptr)&&
                            (lhs.pipelineShaders.vertexShaderModule_ptr == rhs.pipelineShaders.vertexShaderModule_ptr);

            if ((lhs.descriptorSetsCreateInfo_ptrs.size() == rhs.descriptorSetsCreateInfo_ptrs.size()) && isEqual)
                for (size_t i = 0; (i < lhs.descriptorSetsCreateInfo_ptrs.size()) && isEqual; i++)
                {
                    isEqual &= *(lhs.descriptorSetsCreateInfo_ptrs[i]) == *(rhs.descriptorSetsCreateInfo_ptrs[i]);
                }
            else
                return false;

            return isEqual;
        }
    };
}

class PrimitivesPipelines
{
public:
    PrimitivesPipelines(Anvil::BaseDevice* const in_device_ptr);
    ~PrimitivesPipelines();

    Anvil::PipelineID GetPipelineID(PipelineSpecs in_pipelineSpecs);

private:
    Anvil::PipelineID CreatePipeline(PipelineSpecs pipelineSpecs);

private:
    Anvil::BaseDevice* const device_ptr;

    std::unordered_map<PipelineSpecs, Anvil::PipelineID> pipelineSpecsToPipelineID_umap;
	std::vector<Anvil::PipelineID> pipelineIDs;

    std::map<glTFmode, Anvil::PrimitiveTopology> glTFmodeToPrimitiveTopology_map
    {
        {glTFmode::points, Anvil::PrimitiveTopology::POINT_LIST},
        {glTFmode::line, Anvil::PrimitiveTopology::LINE_LIST},
        {glTFmode::line_strip, Anvil::PrimitiveTopology::LINE_STRIP},
        {glTFmode::triangles, Anvil::PrimitiveTopology::TRIANGLE_LIST},
        {glTFmode::triangle_strip, Anvil::PrimitiveTopology::TRIANGLE_STRIP},
        {glTFmode::triangle_fan, Anvil::PrimitiveTopology::TRIANGLE_FAN}
    };

};
