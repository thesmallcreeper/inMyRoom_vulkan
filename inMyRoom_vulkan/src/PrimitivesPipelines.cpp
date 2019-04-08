#include "PrimitivesPipelines.h"

PrimitivesPipelines::PrimitivesPipelines(Anvil::BaseDevice* in_device_ptr)
    : device_ptr(in_device_ptr)
{
}

PrimitivesPipelines::~PrimitivesPipelines()
{
    auto gfx_manager_ptr(device_ptr->get_graphics_pipeline_manager());
    for (Anvil::PipelineID thisPipelineID : pipelineIDs)
        gfx_manager_ptr->delete_pipeline(thisPipelineID);
    pipelineIDs.clear();
}

size_t PrimitivesPipelines::getPipelineIDIndex(const PipelineSpecs in_pipelineSpecs)
{
    auto search = pipelineSpecsToPipelineIDIndex_umap.find(in_pipelineSpecs);
    if (search != pipelineSpecsToPipelineIDIndex_umap.end())
        return search->second;
    Anvil::PipelineID new_pipelineID = createPipeline(in_pipelineSpecs);
    pipelineIDs.push_back(new_pipelineID);
    pipelineSpecsToPipelineIDIndex_umap.emplace(in_pipelineSpecs, pipelineIDs.size() - 1);
    return pipelineIDs.size() - 1;
}

Anvil::PipelineID PrimitivesPipelines::createPipeline(PipelineSpecs in_pipelineSpecs)
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


    pipeline_create_info_ptr->set_descriptor_set_create_info(&in_pipelineSpecs.descriptorSetsCreateInfo_ptrs);


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

    Anvil::PrimitiveTopology thisPrimitiveTopology;
    {
        auto search = glTFmodeToPrimitiveTopology_map.find(in_pipelineSpecs.drawMode);
        thisPrimitiveTopology = search->second;
    }

    pipeline_create_info_ptr->set_primitive_topology(thisPrimitiveTopology);
    pipeline_create_info_ptr->set_rasterization_properties(Anvil::PolygonMode::FILL,
                                                           Anvil::CullModeFlagBits::BACK_BIT,
                                                           Anvil::FrontFace::COUNTER_CLOCKWISE,
                                                           4.0f); /* line_width */
    pipeline_create_info_ptr->toggle_depth_test(true,
                                                Anvil::CompareOp::LESS);
    pipeline_create_info_ptr->toggle_depth_writes(true);

    Anvil::PipelineID pipelineID;
    gfx_manager_ptr->add_pipeline(std::move(pipeline_create_info_ptr),
                                  &pipelineID);

    return pipelineID;
}
