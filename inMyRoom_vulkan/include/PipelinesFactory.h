#pragma once

#include <unordered_map>
#include "hash_combine.h"

#include "misc/graphics_pipeline_create_info.h"
#include "misc/compute_pipeline_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "misc/descriptor_set_create_info.h"
#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/shader_module.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/compute_pipeline_manager.h"

#include "glTFenum.h"

#include "ShadersSetsFamiliesCache.h"

struct PushConstantSpecs
{
    uint32_t offset;
    uint32_t size;
    Anvil::ShaderStageFlags shader_flags;
};

struct ViewportAndScissorsSpecs
{
    int width = -1;
    int height = -1;
};

struct GraphicsPipelineSpecs
{
    bool twoSided = false;
    glTFmode drawMode = static_cast<glTFmode>(-1);

    bool depthWriteEnable = false;
    Anvil::CompareOp depthCompare = Anvil::CompareOp::NEVER;

    glTFcomponentType indexComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType positionComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType normalComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType tangentComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType texcoord0ComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType texcoord1ComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType color0ComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType joints0ComponentType = static_cast<glTFcomponentType>(-1);
    glTFcomponentType weights0ComponentType = static_cast<glTFcomponentType>(-1);

    std::vector<const Anvil::DescriptorSetCreateInfo*> descriptorSetsCreateInfo_ptrs;

    std::vector<PushConstantSpecs> pushConstantSpecs;

    ViewportAndScissorsSpecs viewportAndScissorSpecs;

    ShadersSet pipelineShaders;

    Anvil::RenderPass* renderpass_ptr = nullptr;
    Anvil::SubPassID subpassID = 0;
};

struct ComputePipelineSpecs
{   // TODO push constants
    std::vector<const Anvil::DescriptorSetCreateInfo*> descriptorSetsCreateInfo_ptrs;

    ShadersSet pipelineShaders;
};

namespace std
{
    template <>
    struct hash<GraphicsPipelineSpecs>
    {
        std::size_t operator()(const GraphicsPipelineSpecs& in_pipelineSpecs) const
        {
            std::size_t result = 0;
            hash_combine(result, in_pipelineSpecs.twoSided);
            hash_combine(result, in_pipelineSpecs.drawMode);
            hash_combine(result, in_pipelineSpecs.depthCompare);
            hash_combine(result, in_pipelineSpecs.depthWriteEnable);
            hash_combine(result, in_pipelineSpecs.indexComponentType);
            hash_combine(result, in_pipelineSpecs.positionComponentType);
            hash_combine(result, in_pipelineSpecs.normalComponentType);
            hash_combine(result, in_pipelineSpecs.tangentComponentType);
            hash_combine(result, in_pipelineSpecs.texcoord0ComponentType);
            hash_combine(result, in_pipelineSpecs.texcoord1ComponentType);
            hash_combine(result, in_pipelineSpecs.color0ComponentType);
            hash_combine(result, in_pipelineSpecs.joints0ComponentType);
            hash_combine(result, in_pipelineSpecs.weights0ComponentType);
            hash_combine(result, in_pipelineSpecs.viewportAndScissorSpecs.width);
            hash_combine(result, in_pipelineSpecs.viewportAndScissorSpecs.height);
            hash_combine(result, in_pipelineSpecs.renderpass_ptr);
            hash_combine(result, in_pipelineSpecs.subpassID);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.fragmentShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.geometryShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.tessControlShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.tessEvaluationShaderModule_ptr);
            hash_combine(result, in_pipelineSpecs.pipelineShaders.vertexShaderModule_ptr);

         //   for(const auto& this_descriptorSetCreateInfo_ptr : in_pipelineSpecs.descriptorSetsCreateInfo_ptrs)
         //   {
         //       hash_combine(result, *this_descriptorSetCreateInfo_ptr);
         //   }

         // missing push constant
            return result;
        }
    };

    template <>
    struct hash<ComputePipelineSpecs>
    {
        std::size_t operator()(const ComputePipelineSpecs& in_pipelineSpecs) const
        {
            std::size_t result = 0;
            hash_combine(result, in_pipelineSpecs.pipelineShaders.computeShaderModule_ptr);

         //   for(const auto& this_descriptorSetCreateInfo_ptr : in_pipelineSpecs.descriptorSetsCreateInfo_ptrs)
         //   {
         //       hash_combine(result, *this_descriptorSetCreateInfo_ptr);
         //   }

            return result;
        }
    };

    template <>
    struct equal_to<GraphicsPipelineSpecs>
    {
        bool operator()(const GraphicsPipelineSpecs& lhs, const GraphicsPipelineSpecs& rhs) const
        {
            bool isEqual =  (lhs.twoSided == rhs.twoSided) &&
                            (lhs.drawMode == rhs.drawMode) &&
                            (lhs.depthCompare == rhs.depthCompare) && 
                            (lhs.depthWriteEnable == rhs.depthWriteEnable) &&
                            (lhs.indexComponentType == rhs.indexComponentType) &&
                            (lhs.positionComponentType == rhs.positionComponentType) &&
                            (lhs.normalComponentType == rhs.normalComponentType) &&
                            (lhs.tangentComponentType == rhs.tangentComponentType) &&
                            (lhs.texcoord0ComponentType == rhs.texcoord0ComponentType) &&
                            (lhs.texcoord1ComponentType == rhs.texcoord1ComponentType) &&
                            (lhs.color0ComponentType == rhs.color0ComponentType) &&
                            (lhs.joints0ComponentType == rhs.joints0ComponentType) &&
                            (lhs.weights0ComponentType == rhs.weights0ComponentType) &&
                            (lhs.viewportAndScissorSpecs.width == rhs.viewportAndScissorSpecs.width) &&
                            (lhs.viewportAndScissorSpecs.height == rhs.viewportAndScissorSpecs.height) &&
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

            if ((lhs.pushConstantSpecs.size() == rhs.pushConstantSpecs.size()) && isEqual)
                for (size_t i = 0; (i < lhs.pushConstantSpecs.size()) && isEqual; i++)
                {
                    isEqual &= (lhs.pushConstantSpecs[i].offset == rhs.pushConstantSpecs[i].offset);
                }
            else
                return false;

            return isEqual;
        }
    };

    template <>
    struct equal_to<ComputePipelineSpecs>
    {
        bool operator()(const ComputePipelineSpecs& lhs, const ComputePipelineSpecs& rhs) const
        {
            bool isEqual = (lhs.pipelineShaders.computeShaderModule_ptr == rhs.pipelineShaders.computeShaderModule_ptr);

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

class PipelinesFactory
{
public:  //functions
    PipelinesFactory(Anvil::BaseDevice* const in_device_ptr);
    ~PipelinesFactory();

    Anvil::PipelineID GetGraphicsPipelineID(GraphicsPipelineSpecs in_pipelineSpecs);
    Anvil::PipelineID GetComputePipelineID(ComputePipelineSpecs in_pipelineSpecs);

    VkPipeline GetPipelineVkHandle(Anvil::PipelineBindPoint in_pipeline_bind_point,
                                         Anvil::PipelineID in_pipeline_id) const;

    Anvil::PipelineLayout* GetPipelineLayout(Anvil::PipelineBindPoint in_pipeline_bind_point,
                                                   Anvil::PipelineID in_pipeline_id) const;

private: //functions
    Anvil::PipelineID CreateGraphicsPipeline(GraphicsPipelineSpecs pipelineSpecs);
    Anvil::PipelineID CreateComputePipeline(ComputePipelineSpecs pipelineSpecs);

private: //data
    Anvil::BaseDevice* const device_ptr;

    std::unordered_map<GraphicsPipelineSpecs, Anvil::PipelineID> graphicsPipelineSpecsToPipelineID_umap;
    std::unordered_map<ComputePipelineSpecs, Anvil::PipelineID> computePipelineSpecsToPipelineID_umap;

	std::vector<Anvil::PipelineID> graphicsPipelineIDs;
    std::vector<Anvil::PipelineID> computePipelineIDs;

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
