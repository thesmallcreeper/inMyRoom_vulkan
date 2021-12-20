#include "Graphics/PipelinesFactory.h"

#include <iostream>
#include <cassert>

//
// PipelineLayoutCache
PipelinesFactory::PipelineLayoutCache::PipelineLayoutCache(vk::Device in_device)
    :device(in_device)
{
}

PipelinesFactory::PipelineLayoutCache::~PipelineLayoutCache()
{
    for (auto& this_pair: infoToHandle_umap) {
        device.destroyPipelineLayout(this_pair.second);
    }
}

std::pair<vk::PipelineLayout, const vk::PipelineLayoutCreateInfo *>
    PipelinesFactory::PipelineLayoutCache::GetPipelineLayout(const vk::PipelineLayoutCreateInfo &create_info)
{
    vk::PipelineLayoutCreateInfo this_info;
    assert(create_info.pNext == nullptr);

    // .flags
    this_info.flags = create_info.flags;

    // .layouts
    std::vector<vk::DescriptorSetLayout> data_info_layouts(create_info.pSetLayouts,
                                                          create_info.pSetLayouts + create_info.setLayoutCount);
    auto data_info_layouts_it = descriptorSetsLayout_uset.emplace(std::move(data_info_layouts)).first;
    this_info.setSetLayouts(*data_info_layouts_it);

    // .pushConstantRanges
    std::vector<vk::PushConstantRange> data_info_pushConstantRanges(create_info.pPushConstantRanges,
                                                                    create_info.pPushConstantRanges + create_info.pushConstantRangeCount);
    auto data_info_pushConstantRanges_it = pushConstantRanges_uset.emplace(std::move(data_info_pushConstantRanges)).first;
    this_info.setPushConstantRanges(*data_info_pushConstantRanges_it);


    // Get the pipeline layout
    vk::PipelineLayout return_layout;
    const vk::PipelineLayoutCreateInfo* return_info_ptr;
    auto search = infoToHandle_umap.find(this_info);
    if (search != infoToHandle_umap.end()) {
        return_layout = search->second;
        return_info_ptr = &search->first;
    } else {
        auto layout_opt = device.createPipelineLayout(this_info);
        if (layout_opt.result == vk::Result::eSuccess) {
            auto it = infoToHandle_umap.emplace(this_info, layout_opt.value).first;
            return_layout = it->second;
            return_info_ptr = &it->first;
        } else {
            std::cerr << "Failed to create pipeline layout. \n";
            std::terminate();
        }
    }

    return {return_layout, return_info_ptr};
}

//
// GraphicsPipelineCache
PipelinesFactory::GraphicsPipelineCache::GraphicsPipelineCache(vk::Device in_device)
    :device(in_device)
{
}

PipelinesFactory::GraphicsPipelineCache::~GraphicsPipelineCache()
{
    for (auto& this_pair: infoToHandle_umap) {
        device.destroyPipeline(this_pair.second);
    }
}

std::pair<vk::Pipeline, const vk::GraphicsPipelineCreateInfo *>
PipelinesFactory::GraphicsPipelineCache::GetGraphicsPipeline(const vk::GraphicsPipelineCreateInfo &create_info)
{
    vk::GraphicsPipelineCreateInfo this_info;

    // .pNext
    assert(create_info.pNext == nullptr);

    // .flags
    this_info.flags = create_info.flags;

    // .stages
    assert(create_info.pStages);
    std::vector<vk::PipelineShaderStageCreateInfo> data_info_stages(create_info.pStages,
                                                                    create_info.pStages + create_info.stageCount);
    for(auto& this_stage: data_info_stages) {
        assert(this_stage.pNext == nullptr);
        assert(std::string(this_stage.pName) == "main");
        assert(this_stage.pSpecializationInfo == nullptr);
    }
    auto data_info_stages_it = pipelineShaderStagesCreateInfo_uset.emplace(data_info_stages).first;
    this_info.setStages(*data_info_stages_it);

    // .pVertexInputState
    if (create_info.pVertexInputState != nullptr) {
        vk::PipelineVertexInputStateCreateInfo data_info_vertexInputState;
        {
            assert(create_info.pVertexInputState->pNext == nullptr);

            //.pVertexInputState.flags
            data_info_vertexInputState = create_info.pVertexInputState->flags;

            //.pVertexInputState.vertexBindingDescriptions
            std::vector<vk::VertexInputBindingDescription> data_subinfo_vertexBindings(create_info.pVertexInputState->pVertexBindingDescriptions,
                                                                                    create_info.pVertexInputState->pVertexBindingDescriptions
                                                                                     + create_info.pVertexInputState->vertexBindingDescriptionCount);
            auto data_subinfo_vertexBindings_it = vertexInputBindingDescriptions_uset.emplace(data_subinfo_vertexBindings).first;
            data_info_vertexInputState.setVertexBindingDescriptions(*data_subinfo_vertexBindings_it);

            //.pVertexInputState.vertexAttributeDescriptions
            std::vector<vk::VertexInputAttributeDescription> data_subinfo_vertexAttributes(create_info.pVertexInputState->pVertexAttributeDescriptions,
                                                                                          create_info.pVertexInputState->pVertexAttributeDescriptions
                                                                                           + create_info.pVertexInputState->vertexAttributeDescriptionCount);
            auto data_subinfo_vertexAttributes_it = vertexInputAttributeDescriptions_uset.emplace(data_subinfo_vertexAttributes).first;
            data_info_vertexInputState.setVertexAttributeDescriptions(*data_subinfo_vertexAttributes_it);
        }
        auto data_info_vertexInputState_it = pipelineVertexInputStateCreateInfo_uset.emplace(data_info_vertexInputState).first;
        this_info.pVertexInputState = &*data_info_vertexInputState_it;
    }

    // .pInputAssemblyState
    if (create_info.pInputAssemblyState != nullptr) {
        vk::PipelineInputAssemblyStateCreateInfo data_info_inputAssemblyState = *create_info.pInputAssemblyState;
        {
            assert(data_info_inputAssemblyState.pNext == nullptr);
        }
        auto data_info_inputAssemblyState_it = pipelineInputAssemblyStateCreateInfo_uset.emplace(
                data_info_inputAssemblyState).first;
        this_info.pInputAssemblyState = &*data_info_inputAssemblyState_it;
    }

    // .pTesselationState
    if (create_info.pTessellationState != nullptr) {
        vk::PipelineTessellationStateCreateInfo data_info_tesselationState = *create_info.pTessellationState;
        {
            assert(data_info_tesselationState.pNext == nullptr);
        }
        auto data_info_tesselationState_it = pipelineTesselationStateCreateInfo_uset.emplace(data_info_tesselationState).first;
        this_info.pTessellationState = &*data_info_tesselationState_it;
    }

    // .pViewportState
    if (create_info.pViewportState != nullptr) {
        vk::PipelineViewportStateCreateInfo data_info_viewportState;
        {
            assert(create_info.pViewportState->pNext == nullptr);

            // .pPipelineViewport.flags
            data_info_viewportState.flags = create_info.pViewportState->flags;

            // .pPipelineViewport.viewports
            std::vector<vk::Viewport> data_subinfo_vieports(create_info.pViewportState->pViewports,
                                                           create_info.pViewportState->pViewports + create_info.pViewportState->viewportCount);
            auto data_subinfo_vieports_it = viewports_uset.emplace(data_subinfo_vieports).first;
            data_info_viewportState.setViewports(*data_subinfo_vieports_it);

            // .pPipelineViewport.scissors
            std::vector<vk::Rect2D> data_subinfo_scissors(create_info.pViewportState->pScissors,
                                                          create_info.pViewportState->pScissors + create_info.pViewportState->scissorCount);
            auto data_subinfo_scissors_it = scissors_uset.emplace(data_subinfo_scissors).first;
            data_info_viewportState.setScissors(*data_subinfo_scissors_it);
        }
        auto data_info_viewportState_it = pipelineViewportStatesCreateInfo_uset.emplace(data_info_viewportState).first;
        this_info.pViewportState = &*data_info_viewportState_it;
    }

    // .pRasterizationState
    assert(create_info.pRasterizationState);
    vk::PipelineRasterizationStateCreateInfo data_info_rasterizationState = *create_info.pRasterizationState;
    {
        assert(data_info_rasterizationState.pNext == nullptr);
    }
    auto data_info_rasterizationState_it = pipelineRasterizationStateCreateInfo_uset.emplace(data_info_rasterizationState).first;
    this_info.pRasterizationState = &*data_info_rasterizationState_it;

    // .pMultisampleState
    if (create_info.pMultisampleState != nullptr) {
        vk::PipelineMultisampleStateCreateInfo data_info_multisampleState = *create_info.pMultisampleState;
        {
            assert(data_info_multisampleState.pNext == nullptr);
        }
        auto data_info_multisampleState_it = pipelineMultisampleStateCreateInfo_uset.emplace(data_info_multisampleState).first;
        this_info.pMultisampleState = &*data_info_multisampleState_it;
    }

    // .pDepthStencilState
    if (create_info.pDepthStencilState != nullptr) {
        vk::PipelineDepthStencilStateCreateInfo data_info_depthStencilState = *create_info.pDepthStencilState;
        {
            assert(data_info_depthStencilState.pNext == nullptr);
        }
        auto data_info_depthStencilState_it = pipelineDepthStencilStateCreateInfo_uset.emplace(data_info_depthStencilState).first;
        this_info.pDepthStencilState = &*data_info_depthStencilState_it;
    }

    // .pColorBlendState
    if (create_info.pColorBlendState != nullptr) {
        vk::PipelineColorBlendStateCreateInfo data_info_colorBlendState;
        {
            assert(create_info.pColorBlendState->pNext == nullptr);

            // .pColorBlendState.flags
            data_info_colorBlendState.flags = create_info.pColorBlendState->flags;

            // .pColorBlendState.logicOpEnable
            data_info_colorBlendState.logicOpEnable = create_info.pColorBlendState->logicOpEnable;

            // .pColorBlendState.logicOp
            data_info_colorBlendState.logicOp = create_info.pColorBlendState->logicOp;

            // .pColorBlendState.attachments
            std::vector<vk::PipelineColorBlendAttachmentState> data_subinfo_colorBlendState(create_info.pColorBlendState->pAttachments,
                                                                                            create_info.pColorBlendState->pAttachments
                                                                                             + create_info.pColorBlendState->attachmentCount);
            auto data_subinfo_colorBlendState_it = pipelineColorBlendAttachmentStates_uset.emplace(data_subinfo_colorBlendState).first;
            data_info_colorBlendState.setAttachments(*data_subinfo_colorBlendState_it);

            // .pColorBlendState.blendConstants
            std::array<float, 4> data_subinfo_blendConstant = {create_info.pColorBlendState->blendConstants[0],
                                                               create_info.pColorBlendState->blendConstants[1],
                                                               create_info.pColorBlendState->blendConstants[2],
                                                               create_info.pColorBlendState->blendConstants[3]};
            auto data_subinfo_blendConstants_it = pipelineColorBlendConstants_uset.emplace(data_subinfo_blendConstant).first;
            data_info_colorBlendState.setBlendConstants(*data_subinfo_blendConstants_it);
        }
        auto data_info_colorBlendState_it = pipelineColorBlendStateCreateInfo_uset.emplace(data_info_colorBlendState).first;
        this_info.pColorBlendState = &*data_info_colorBlendState_it;
    }

    // .pDynamicState
    if (create_info.pDynamicState) {
        vk::PipelineDynamicStateCreateInfo data_info_dynamicState;
        {
            assert(create_info.pDynamicState->pNext == nullptr);

            // .pDynamicState.flags
            data_info_dynamicState.flags = create_info.pDynamicState->flags;

            // .pDynamicState.dynamicStates
            std::vector<vk::DynamicState> data_subinfo_dynamicState(create_info.pDynamicState->pDynamicStates,
                                                                    create_info.pDynamicState->pDynamicStates
                                                                     + create_info.pDynamicState->dynamicStateCount);
            auto data_subinfo_dynamicState_it = dynamicStates_uset.emplace(data_subinfo_dynamicState).first;
            data_info_dynamicState.setDynamicStates(*data_subinfo_dynamicState_it);
        }
        auto data_info_dynamicState_it = pipelineDynamicStatesCreateInfo_uset.emplace(data_info_dynamicState).first;
        this_info.pDynamicState = &*data_info_dynamicState_it;
    }

    // .layout
    this_info.layout = create_info.layout;

    // .renderPass
    this_info.renderPass = create_info.renderPass;
    this_info.subpass = create_info.subpass;

    // .basePipelineHandle
    this_info.basePipelineHandle = create_info.basePipelineHandle;

    // .basePipelineIndex
    this_info.basePipelineIndex = create_info.basePipelineIndex;

    // Get Pipeline
    vk::Pipeline return_pipeline;
    const vk::GraphicsPipelineCreateInfo* return_info_ptr;
    auto search = infoToHandle_umap.find(this_info);
    if (search != infoToHandle_umap.end()) {
        return_pipeline = search->second;
        return_info_ptr = &search->first;
    } else {
        auto pipeline_opt = device.createGraphicsPipeline( VK_NULL_HANDLE, this_info);
        if (pipeline_opt.result == vk::Result::eSuccess) {
            auto it = infoToHandle_umap.emplace(this_info, pipeline_opt.value).first;
            return_pipeline = it->second;
            return_info_ptr = &it->first;
        } else {
            std::cerr << "Failed to create graphics pipeline. \n";
            std::terminate();
        }
    }

    return {return_pipeline, return_info_ptr};
}

PipelinesFactory::ComputePipelineCache::ComputePipelineCache(vk::Device in_device)
    :device(in_device)
{
}

PipelinesFactory::ComputePipelineCache::~ComputePipelineCache()
{
    for (auto& this_pair: infoToHandle_umap) {
        device.destroyPipeline(this_pair.second);
    }
}

std::pair<vk::Pipeline, const vk::ComputePipelineCreateInfo *>
    PipelinesFactory::ComputePipelineCache::GetComputePipeline(const vk::ComputePipelineCreateInfo &create_info)
{
    assert(create_info.pNext == nullptr);

    vk::Pipeline return_pipeline;
    const vk::ComputePipelineCreateInfo* return_info_ptr;
    auto search = infoToHandle_umap.find(create_info);
    if (search != infoToHandle_umap.end()) {
        return_pipeline = search->second;
        return_info_ptr = &search->first;
    } else {
        auto pipeline_opt = device.createComputePipeline( VK_NULL_HANDLE, create_info);
        if (pipeline_opt.result == vk::Result::eSuccess) {
            auto it = infoToHandle_umap.emplace(create_info, pipeline_opt.value).first;
            return_pipeline = it->second;
            return_info_ptr = &it->first;
        } else {
            std::cerr << "Failed to create compute pipeline. \n";
            std::terminate();
        }
    }

    return {return_pipeline, return_info_ptr};
}

PipelinesFactory::PipelinesFactory(vk::Device in_device)
    :pipelineLayoutCache(in_device),
     graphicsPipelineCache(in_device),
     computePipelineCache(in_device)
{
}

std::pair<vk::PipelineLayout, const vk::PipelineLayoutCreateInfo *>
    PipelinesFactory::GetPipelineLayout(const vk::PipelineLayoutCreateInfo &create_info)
{
    return pipelineLayoutCache.GetPipelineLayout(create_info);
}

std::pair<vk::Pipeline, const vk::GraphicsPipelineCreateInfo *>
    PipelinesFactory::GetPipeline(const vk::GraphicsPipelineCreateInfo &create_info)
{
    return graphicsPipelineCache.GetGraphicsPipeline(create_info);
}

std::pair<vk::Pipeline, const vk::ComputePipelineCreateInfo *>
    PipelinesFactory::GetPipeline(const vk::ComputePipelineCreateInfo &create_info)
{
    return computePipelineCache.GetComputePipeline(create_info);
}

