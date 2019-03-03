#pragma once
#include <cassert>

#include <unordered_map>

#include "misc/base_pipeline_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "wrappers/device.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/shader_module.h"
#include "wrappers/graphics_pipeline_manager.h"

#include "glTFenum.h"

template <class T>
inline void hash_combine(std::size_t & s, const T & v)
{
	std::hash<T> h;
	s ^= h(v) + 0x9e3779b9 + (s << 6) + (s >> 2);
}

struct PipelineSpecs
{
	glTFmode drawMode = static_cast<glTFmode>(-1);
	glTFcomponentType indexComponentType = static_cast<glTFcomponentType>(-1);
	glTFcomponentType positionComponentType = static_cast<glTFcomponentType>(-1);
	glTFcomponentType normalComponentType = static_cast<glTFcomponentType>(-1);
	glTFcomponentType tangentComponentType = static_cast<glTFcomponentType>(-1);
	glTFcomponentType textcoord0ComponentType = static_cast<glTFcomponentType>(-1);
	Anvil::DescriptorSetGroup* dsg_ptr;
	Anvil::RenderPass* renderpass_ptr;
	Anvil::SubPassID subpassID;
};

namespace std
{
	template <> struct hash<PipelineSpecs>
	{
		std::size_t operator()(const PipelineSpecs& in_pipelineSpecs) const
		{
			std::size_t result = 0;
			hash_combine(result, in_pipelineSpecs.drawMode);
			hash_combine(result, in_pipelineSpecs.indexComponentType);
			hash_combine(result, in_pipelineSpecs.positionComponentType);
			hash_combine(result, in_pipelineSpecs.normalComponentType);
			hash_combine(result, in_pipelineSpecs.tangentComponentType);
			hash_combine(result, in_pipelineSpecs.dsg_ptr);
			hash_combine(result, in_pipelineSpecs.renderpass_ptr);
			hash_combine(result, in_pipelineSpecs.subpassID);
			
			return result;
		}
	};

	template <> struct equal_to<PipelineSpecs>
	{
		constexpr bool operator()(const PipelineSpecs& lhs, const PipelineSpecs& rhs) const
		{
			return (lhs.drawMode == rhs.drawMode) &
				(lhs.indexComponentType == rhs.indexComponentType) &
				(lhs.positionComponentType == rhs.positionComponentType) &
				(lhs.normalComponentType == rhs.normalComponentType) &
				(lhs.tangentComponentType == rhs.tangentComponentType) &
				(lhs.dsg_ptr == rhs.dsg_ptr) &
				(lhs.renderpass_ptr == rhs.renderpass_ptr) &
				(lhs.subpassID == rhs.subpassID);
		}
	};
}

class PrimitivesPipelines
{
public:
	PrimitivesPipelines(Anvil::ShaderModuleStageEntryPoint* in_vs_ptr, Anvil::ShaderModuleStageEntryPoint* in_fs_ptr);
	~PrimitivesPipelines();

	void deinit(Anvil::BaseDevice* in_device_ptr);

	size_t getPipelineIDIndex(const PipelineSpecs in_pipelineSpecs, Anvil::BaseDevice* in_device_ptr);

	std::vector<Anvil::PipelineID> pipelineIDs;

private:
	Anvil::PipelineID createPipeline(PipelineSpecs pipelineSpecs, Anvil::BaseDevice* in_device_ptr);

	std::unordered_map<PipelineSpecs, size_t> pipelineSpecsToPipelineIDIndex_umap;

	Anvil::ShaderModuleStageEntryPoint* vs_ptr;
	Anvil::ShaderModuleStageEntryPoint* fs_ptr;

	std::map<glTFmode, Anvil::PrimitiveTopology> glTFmodeToPrimitiveTopology_map
	{
		{glTFmode::points, Anvil::PrimitiveTopology::POINT_LIST},
		{glTFmode::line, Anvil::PrimitiveTopology::LINE_LIST},
		{glTFmode::line_strip, Anvil::PrimitiveTopology::LINE_STRIP},
		{glTFmode::triangles,Anvil::PrimitiveTopology::TRIANGLE_LIST},
		{glTFmode::triangle_strip, Anvil::PrimitiveTopology::TRIANGLE_STRIP},
		{glTFmode::triangle_fan, Anvil::PrimitiveTopology::TRIANGLE_FAN}
	};
};