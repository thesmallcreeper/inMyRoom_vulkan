#include "PipelinesFactory.h"

PipelinesFactory::PipelinesFactory(Anvil::BaseDevice* const in_device_ptr)
    : device_ptr(in_device_ptr)
{
}

PipelinesFactory::~PipelinesFactory()
{
    {
        auto gfx_manager_ptr(device_ptr->get_graphics_pipeline_manager());
        for (Anvil::PipelineID thisPipelineID : graphicsPipelineIDs)
            gfx_manager_ptr->delete_pipeline(thisPipelineID);
    }
    {
        auto compute_manager_ptr(device_ptr->get_compute_pipeline_manager());
        for (Anvil::PipelineID thisPipelineID : computePipelineIDs)
            compute_manager_ptr->delete_pipeline(thisPipelineID);
    }
}

Anvil::PipelineID PipelinesFactory::getGraphicsPipelineID(const GraphicsPipelineSpecs in_pipelineSpecs)
{
    auto search = graphicsPipelineSpecsToPipelineID_umap.find(in_pipelineSpecs);
	if (search != graphicsPipelineSpecsToPipelineID_umap.end())
	{
		return search->second;
	}
	else
	{
		Anvil::PipelineID new_pipelineID = createGraphicsPipeline(in_pipelineSpecs);
		graphicsPipelineSpecsToPipelineID_umap.emplace(in_pipelineSpecs, new_pipelineID);
		return new_pipelineID;
	}
}

Anvil::PipelineID PipelinesFactory::getComputePipelineID(const ComputePipelineSpecs in_pipelineSpecs)
{
    auto search = computePipelineSpecsToPipelineID_umap.find(in_pipelineSpecs);
    if (search != computePipelineSpecsToPipelineID_umap.end())
    {
        return search->second;
    }
    else
    {
        Anvil::PipelineID new_pipelineID = createComputePipeline(in_pipelineSpecs);
        computePipelineSpecsToPipelineID_umap.emplace(in_pipelineSpecs, new_pipelineID);
        return new_pipelineID;
    }
}

VkPipeline PipelinesFactory::getPipelineVkHandle(Anvil::PipelineBindPoint in_pipeline_bind_point,
                                                      Anvil::PipelineID in_pipeline_id) const
{
    VkPipeline pipeline_vk = (in_pipeline_bind_point == Anvil::PipelineBindPoint::COMPUTE) ? device_ptr->get_compute_pipeline_manager()->get_pipeline(in_pipeline_id)
                                                                                           : device_ptr->get_graphics_pipeline_manager()->get_pipeline(in_pipeline_id);

    return pipeline_vk;
}

Anvil::PipelineLayout* PipelinesFactory::getPipelineLayout(Anvil::PipelineBindPoint in_pipeline_bind_point,
                                                                Anvil::PipelineID in_pipeline_id) const
{
    Anvil::PipelineLayout* pipeline_layout = (in_pipeline_bind_point == Anvil::PipelineBindPoint::COMPUTE) ? device_ptr->get_compute_pipeline_manager()->get_pipeline_layout(in_pipeline_id)
                                                                                                           : device_ptr->get_graphics_pipeline_manager()->get_pipeline_layout(in_pipeline_id);

    return pipeline_layout;
}

Anvil::PipelineID PipelinesFactory::createGraphicsPipeline(GraphicsPipelineSpecs in_pipelineSpecs)
{
    auto gfx_manager_ptr(device_ptr->get_graphics_pipeline_manager());

    auto pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                              in_pipelineSpecs.renderpass_ptr,
                                                                              in_pipelineSpecs.subpassID,
                                                                              in_pipelineSpecs.pipelineShaders.fragmentShaderModule_ptr!= nullptr
                                                                                  ? *in_pipelineSpecs.pipelineShaders.fragmentShaderModule_ptr
                                                                                  : Anvil::ShaderModuleStageEntryPoint(),
                                                                              in_pipelineSpecs.pipelineShaders.geometryShaderModule_ptr!= nullptr
                                                                                  ? *in_pipelineSpecs.pipelineShaders.geometryShaderModule_ptr
                                                                                  : Anvil::ShaderModuleStageEntryPoint(),
                                                                              in_pipelineSpecs.pipelineShaders.tessControlShaderModule_ptr != nullptr
                                                                                  ? *in_pipelineSpecs.pipelineShaders.tessControlShaderModule_ptr
                                                                                  : Anvil::ShaderModuleStageEntryPoint(),
                                                                              in_pipelineSpecs.pipelineShaders.tessEvaluationShaderModule_ptr != nullptr
                                                                                  ? *in_pipelineSpecs.pipelineShaders.tessEvaluationShaderModule_ptr
                                                                                  : Anvil::ShaderModuleStageEntryPoint(),
                                                                              in_pipelineSpecs.pipelineShaders.vertexShaderModule_ptr != nullptr
                                                                                  ? *in_pipelineSpecs.pipelineShaders.vertexShaderModule_ptr
                                                                                  : Anvil::ShaderModuleStageEntryPoint());

    // TODO: Add option at creation
    pipeline_create_info_ptr->attach_push_constant_range(0                                          /*in_offset*/,
                                                         sizeof(float) * 4 * 4                      /*in_size*/,
                                                         Anvil::ShaderStageFlagBits::VERTEX_BIT     /*in_stages*/);

    if (in_pipelineSpecs.descriptorSetsCreateInfo_ptrs.size())
    {
        pipeline_create_info_ptr->set_descriptor_set_create_info(&in_pipelineSpecs.descriptorSetsCreateInfo_ptrs);
    }

    uint32_t location = 0;

    // vertex attribute

    if (in_pipelineSpecs.positionComponentType == glTFcomponentType::type_float)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R32G32B32_SFLOAT,
                                                       0,
                                                       sizeof(float) * 3,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }

    // normal attribute

    if (in_pipelineSpecs.normalComponentType == glTFcomponentType::type_float)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R32G32B32_SFLOAT,
                                                       0,
                                                       sizeof(float) * 3,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }

    // tangent attribute

    if (in_pipelineSpecs.tangentComponentType == glTFcomponentType::type_float)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R32G32B32A32_SFLOAT,
                                                       0,
                                                       sizeof(float) * 4,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }

    // texcoord_0 attribute

    if (in_pipelineSpecs.texcoord0ComponentType == glTFcomponentType::type_float)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R32G32_SFLOAT,
                                                       0,
                                                       sizeof(float) * 2,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }
    else if (in_pipelineSpecs.texcoord0ComponentType == glTFcomponentType::type_unsigned_byte)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R8G8_UNORM,
                                                       0,
                                                       sizeof(uint8_t) * 2,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }
    else if (in_pipelineSpecs.texcoord0ComponentType == glTFcomponentType::type_unsigned_short)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R16G16_UNORM,
                                                       0,
                                                       sizeof(uint16_t) * 2,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }

    // texcoord_1 attribute

    if (in_pipelineSpecs.texcoord1ComponentType == glTFcomponentType::type_float)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R32G32_SFLOAT,
                                                       0,
                                                       sizeof(float) * 2,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }
    else if (in_pipelineSpecs.texcoord1ComponentType == glTFcomponentType::type_unsigned_byte)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R8G8_UNORM,
                                                       0,
                                                       sizeof(uint8_t) * 2,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }
    else if (in_pipelineSpecs.texcoord1ComponentType == glTFcomponentType::type_unsigned_short)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R16G16_UNORM,
                                                       0,
                                                       sizeof(uint16_t) * 2,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }

    // color_0 attribute

    if (in_pipelineSpecs.color0ComponentType == glTFcomponentType::type_float)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R32G32B32A32_SFLOAT,
                                                       0,
                                                       sizeof(float) * 4,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }
    else if (in_pipelineSpecs.color0ComponentType == glTFcomponentType::type_unsigned_byte)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R8G8B8A8_UNORM,
                                                       0,
                                                       sizeof(uint8_t) * 4,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }
    else if (in_pipelineSpecs.color0ComponentType == glTFcomponentType::type_unsigned_short)
    {
        pipeline_create_info_ptr->add_vertex_attribute(location,
                                                       Anvil::Format::R16G16B16A16_UNORM,
                                                       0,
                                                       sizeof(uint16_t) * 4,
                                                       Anvil::VertexInputRate::VERTEX,
                                                       location);

        location++;
    }


    Anvil::PrimitiveTopology thisPrimitiveTopology;
    {
        auto search = glTFmodeToPrimitiveTopology_map.find(in_pipelineSpecs.drawMode);
        thisPrimitiveTopology = search->second;
    }

    pipeline_create_info_ptr->set_primitive_topology(thisPrimitiveTopology);
    pipeline_create_info_ptr->set_rasterization_properties(Anvil::PolygonMode::FILL,
                                                           Anvil::CullModeFlagBits::BACK_BIT,
                                                           Anvil::FrontFace::COUNTER_CLOCKWISE,
                                                           1.0f); /* line_width */

    if (in_pipelineSpecs.depthCompare != Anvil::CompareOp::NEVER)
    {
        pipeline_create_info_ptr->toggle_depth_test(true,
                                                    in_pipelineSpecs.depthCompare);
    }
    else
    {
        pipeline_create_info_ptr->toggle_depth_test(false,
                                                    Anvil::CompareOp::NEVER);
    }
    pipeline_create_info_ptr->toggle_depth_writes(in_pipelineSpecs.depthWriteEnable);

    Anvil::PipelineID pipelineID;
    gfx_manager_ptr->add_pipeline(std::move(pipeline_create_info_ptr),
                                  &pipelineID);

	graphicsPipelineIDs.push_back(pipelineID);

    return pipelineID;
}

Anvil::PipelineID PipelinesFactory::createComputePipeline(ComputePipelineSpecs in_pipelineSpecs)
{
    auto compute_manager_ptr(device_ptr->get_graphics_pipeline_manager());

    auto pipeline_create_info_ptr = Anvil::ComputePipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
                                                                             *in_pipelineSpecs.pipelineShaders.computeShaderModule_ptr);

    if (in_pipelineSpecs.descriptorSetsCreateInfo_ptrs.size())
    {
        pipeline_create_info_ptr->set_descriptor_set_create_info(&in_pipelineSpecs.descriptorSetsCreateInfo_ptrs);
    }

    Anvil::PipelineID pipelineID;
    compute_manager_ptr->add_pipeline(std::move(pipeline_create_info_ptr),
                                      &pipelineID);

    computePipelineIDs.push_back(pipelineID);

    return pipelineID;
}