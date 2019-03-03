#include "PrimitivesPipelines.h"

PrimitivesPipelines::PrimitivesPipelines(Anvil::ShaderModuleStageEntryPoint* in_vs_ptr, Anvil::ShaderModuleStageEntryPoint* in_fs_ptr)
	:vs_ptr(in_vs_ptr),
	 fs_ptr(in_fs_ptr)
{

}

PrimitivesPipelines::~PrimitivesPipelines()
{
	// Should deinit before deleting
	assert(pipelineIDs.size() == 0);
}

void PrimitivesPipelines::deinit(Anvil::BaseDevice* in_device_ptr)
{
	auto gfx_manager_ptr(in_device_ptr->get_graphics_pipeline_manager());
	for (Anvil::PipelineID thisPipelineID : pipelineIDs)
		gfx_manager_ptr->delete_pipeline(thisPipelineID);
	pipelineIDs.clear();
}

size_t PrimitivesPipelines::getPipelineIDIndex(const PipelineSpecs in_pipelineSpecs, Anvil::BaseDevice* in_device_ptr)
{
	auto search = pipelineSpecsToPipelineIDIndex_umap.find(in_pipelineSpecs);
	if (search != pipelineSpecsToPipelineIDIndex_umap.end())
		return search->second;
	else
	{
		Anvil::PipelineID newPipelineID = createPipeline(in_pipelineSpecs, in_device_ptr);
		pipelineIDs.push_back(newPipelineID);
		pipelineSpecsToPipelineIDIndex_umap.emplace(in_pipelineSpecs, pipelineIDs.size() - 1);
		return pipelineIDs.size() - 1;
	}
}

Anvil::PipelineID PrimitivesPipelines::createPipeline(PipelineSpecs in_pipelineSpecs, Anvil::BaseDevice* in_device_ptr)
{
	auto gfx_manager_ptr(in_device_ptr->get_graphics_pipeline_manager());

	auto pipeline_create_info_ptr = Anvil::GraphicsPipelineCreateInfo::create(Anvil::PipelineCreateFlagBits::NONE,
																			  in_pipelineSpecs.renderpass_ptr,
																			  in_pipelineSpecs.subpassID,
																			  *fs_ptr,
																			  Anvil::ShaderModuleStageEntryPoint(), /* in_gs_entrypoint */
																			  Anvil::ShaderModuleStageEntryPoint(), /* in_tc_entrypoint */
																			  Anvil::ShaderModuleStageEntryPoint(), /* in_te_entrypoint */
																			  *vs_ptr);

	pipeline_create_info_ptr->set_descriptor_set_create_info(in_pipelineSpecs.dsg_ptr->get_descriptor_set_create_info());

	if (in_pipelineSpecs.positionComponentType == glTFcomponentType::type_float)
	{
		pipeline_create_info_ptr->add_vertex_attribute(0,
													   Anvil::Format::R32G32B32_SFLOAT,
													   0,    
													   sizeof(float) * 3, 
													   Anvil::VertexInputRate::VERTEX);
	}


	if (in_pipelineSpecs.normalComponentType == glTFcomponentType::type_float)
	{
		pipeline_create_info_ptr->add_vertex_attribute(1, 
													   Anvil::Format::R32G32B32_SFLOAT,
													   0,         
													   sizeof(float) * 3, 
													   Anvil::VertexInputRate::VERTEX);
	}


	if (in_pipelineSpecs.tangentComponentType == glTFcomponentType::type_float)
	{
		pipeline_create_info_ptr->add_vertex_attribute(2,
													   Anvil::Format::R32G32B32A32_SFLOAT,
													   0,
													   sizeof(float) * 4,
													   Anvil::VertexInputRate::VERTEX);
	}


	if (in_pipelineSpecs.textcoord0ComponentType == glTFcomponentType::type_float)
	{
		pipeline_create_info_ptr->add_vertex_attribute(3,
													   Anvil::Format::R32G32_SFLOAT,
													   0,
													   sizeof(float) * 2,
													   Anvil::VertexInputRate::VERTEX);
	}
		else if (in_pipelineSpecs.textcoord0ComponentType == glTFcomponentType::type_unsigned_byte)
		{
			pipeline_create_info_ptr->add_vertex_attribute(3,
														   Anvil::Format::R8G8_UNORM,
														   0,
														   sizeof(uint8_t) * 2,
														   Anvil::VertexInputRate::VERTEX);
		}
		else if (in_pipelineSpecs.textcoord0ComponentType == glTFcomponentType::type_unsigned_short)
		{
			pipeline_create_info_ptr->add_vertex_attribute(3,
														   Anvil::Format::R16G16_UNORM,
														   0,
														   sizeof(uint16_t) * 2,
														   Anvil::VertexInputRate::VERTEX);
		}

	auto search = glTFmodeToPrimitiveTopology_map.find(in_pipelineSpecs.drawMode);
	
	pipeline_create_info_ptr->set_primitive_topology(search->second);
	pipeline_create_info_ptr->set_rasterization_properties(Anvil::PolygonMode::FILL,
														   Anvil::CullModeFlagBits::BACK_BIT,
														   Anvil::FrontFace::CLOCKWISE,
														   4.0f); /* line_width */
	pipeline_create_info_ptr->toggle_depth_test(true, /* should_enable */
												Anvil::CompareOp::LESS);
	pipeline_create_info_ptr->toggle_depth_writes(true);

	Anvil::PipelineID pipelineID;
	gfx_manager_ptr->add_pipeline(std::move(pipeline_create_info_ptr),
								  &pipelineID);

	return pipelineID;
}