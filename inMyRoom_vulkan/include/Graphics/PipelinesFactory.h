#pragma once

#include <vector>
#include <unordered_set>
#include <unordered_map>
#include "hash_combine.h"

#include "vulkan/vulkan.hpp"
#include "glTFenum.h"

class PipelinesFactory
{
    class PipelineLayoutCache
    {
    public:
        explicit PipelineLayoutCache(vk::Device device);
        ~PipelineLayoutCache();

        std::pair<vk::PipelineLayout, const vk::PipelineLayoutCreateInfo*> GetPipelineLayout(const vk::PipelineLayoutCreateInfo& create_info);
    private:
        vk::Device device;

        std::unordered_map<vk::PipelineLayoutCreateInfo, vk::PipelineLayout>    infoToHandle_umap;
        std::unordered_set<std::vector<vk::DescriptorSetLayout>>                descriptorSetsLayout_uset;
        std::unordered_set<std::vector<vk::PushConstantRange>>                  pushConstantRanges_uset;
    };

    class GraphicsPipelineCache
    {
    public:
        explicit GraphicsPipelineCache(vk::Device device);
        ~GraphicsPipelineCache();

        std::pair<vk::Pipeline, const vk::GraphicsPipelineCreateInfo*> GetGraphicsPipeline(const vk::GraphicsPipelineCreateInfo& create_info);
    private:
        vk::Device device;

        std::unordered_map<vk::GraphicsPipelineCreateInfo, vk::Pipeline>        infoToHandle_umap;

        std::unordered_set<std::vector<vk::PipelineShaderStageCreateInfo>>      pipelineShaderStagesCreateInfo_uset;
        std::unordered_set<vk::PipelineVertexInputStateCreateInfo>              pipelineVertexInputStateCreateInfo_uset;
         std::unordered_set<std::vector<vk::VertexInputBindingDescription>>      vertexInputBindingDescriptions_uset;
         std::unordered_set<std::vector<vk::VertexInputAttributeDescription>>    vertexInputAttributeDescriptions_uset;
        std::unordered_set<vk::PipelineInputAssemblyStateCreateInfo>            pipelineInputAssemblyStateCreateInfo_uset;
        std::unordered_set<vk::PipelineTessellationStateCreateInfo>             pipelineTesselationStateCreateInfo_uset;
        std::unordered_set<vk::PipelineViewportStateCreateInfo>                 pipelineViewportStatesCreateInfo_uset;
         std::unordered_set<std::vector<vk::Viewport>>                           viewports_uset;
         std::unordered_set<std::vector<vk::Rect2D>>                             scissors_uset;
        std::unordered_set<vk::PipelineRasterizationStateCreateInfo>            pipelineRasterizationStateCreateInfo_uset;
        std::unordered_set<vk::PipelineMultisampleStateCreateInfo>              pipelineMultisampleStateCreateInfo_uset;
        std::unordered_set<vk::PipelineDepthStencilStateCreateInfo>             pipelineDepthStencilStateCreateInfo_uset;
        std::unordered_set<vk::PipelineColorBlendStateCreateInfo>               pipelineColorBlendStateCreateInfo_uset;
         std::unordered_set<std::vector<vk::PipelineColorBlendAttachmentState>>  pipelineColorBlendAttachmentStates_uset;
         std::unordered_set<std::array<float, 4>>                                pipelineColorBlendConstants_uset;
        std::unordered_set<vk::PipelineDynamicStateCreateInfo>                  pipelineDynamicStatesCreateInfo_uset;
         std::unordered_set<std::vector<vk::DynamicState>>                       dynamicStates_uset;
    };

    class ComputePipelineCache
    {
    public:
        explicit ComputePipelineCache(vk::Device device);
        ~ComputePipelineCache();

        std::pair<vk::Pipeline, const vk::ComputePipelineCreateInfo*> GetComputePipeline(const vk::ComputePipelineCreateInfo& create_info);
    private:
        vk::Device device;

        std::unordered_map<vk::ComputePipelineCreateInfo, vk::Pipeline>         infoToHandle_umap;
    };
public:  //functions
    explicit PipelinesFactory(vk::Device in_device);

    std::pair<vk::PipelineLayout, const vk::PipelineLayoutCreateInfo*> GetPipelineLayout(const vk::PipelineLayoutCreateInfo& create_info);

    std::pair<vk::Pipeline, const vk::GraphicsPipelineCreateInfo*> GetPipeline(const vk::GraphicsPipelineCreateInfo& create_info);
    std::pair<vk::Pipeline, const vk::ComputePipelineCreateInfo*> GetPipeline(const vk::ComputePipelineCreateInfo& create_info);

private: //data
    PipelineLayoutCache pipelineLayoutCache;

    GraphicsPipelineCache graphicsPipelineCache;
    ComputePipelineCache computePipelineCache;
};
