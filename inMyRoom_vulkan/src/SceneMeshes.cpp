#include "SceneMeshes.h"

SceneMeshes::SceneMeshes(tinygltf::Model& in_model, PrimitivesPipelines& in_pipelinesOfPrimitives,
						 Anvil::DescriptorSetGroup* in_dsg_ptr, Anvil::RenderPass* in_renderpass_ptr, Anvil::SubPassID in_subpassID, Anvil::BaseDevice* in_device_ptr)
	:pipelinesOfPrimitives(in_pipelinesOfPrimitives)
{

	size_t primitiveCount(0);
	for (tinygltf::Mesh& thisMesh : in_model.meshes)
	{
		MeshRange thisMeshRange{ primitiveCount, thisMesh.primitives.size() };
		primitiveCount += thisMesh.primitives.size();

		meshes.emplace_back(thisMeshRange);

		for (tinygltf::Primitive& thisPrimitive : thisMesh.primitives)
		{
			PrimitiveInfo thisPrimitiveInfo;

			PipelineSpecs pipelineSpecs;
			pipelineSpecs.drawMode = static_cast<glTFmode>(thisPrimitive.mode);

			{
				tinygltf::Accessor& thisAccessor = in_model.accessors[thisPrimitive.indices];
				Anvil::BufferUniquePtr bufferUniqueID = CreateBufferForBufferViewAndCopy(in_model, in_model.bufferViews[thisAccessor.bufferView], Anvil::BufferUsageFlagBits::INDEX_BUFFER_BIT, in_device_ptr);

				thisPrimitiveInfo.indicesCount = static_cast<uint32_t>(thisAccessor.count);

				if (thisAccessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short))
					thisPrimitiveInfo.indexBufferType = Anvil::IndexType::UINT16;
				else if (thisAccessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_int))
					thisPrimitiveInfo.indexBufferType = Anvil::IndexType::UINT32;
				else
					assert(0);

				thisPrimitiveInfo.indexBufferIndex = static_cast<int32_t>(indexBuffers.size());
				thisPrimitiveInfo.BufferOffsets.indexBufferOffset = thisAccessor.byteOffset;

				indexBuffers.emplace_back(std::move(bufferUniqueID));

				pipelineSpecs.indexComponentType = static_cast<glTFcomponentType>(thisAccessor.componentType);
			}

			auto searchPositionAttribute = thisPrimitive.attributes.find("POSITION");
			if (searchPositionAttribute != thisPrimitive.attributes.end())
			{
				int thisPositionAttribute = searchPositionAttribute->second;
				tinygltf::Accessor& thisAccessor = in_model.accessors[thisPositionAttribute];
				Anvil::BufferUniquePtr bufferUniqueID = CreateBufferForBufferViewAndCopy(in_model, in_model.bufferViews[thisAccessor.bufferView], Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, in_device_ptr);

				if (thisAccessor.componentType != static_cast<int>(glTFcomponentType::type_float))
					assert(0);

				thisPrimitiveInfo.positionBufferIndex = static_cast<uint32_t>(positionBuffers.size());
				thisPrimitiveInfo.BufferOffsets.positionBufferOffset = thisAccessor.byteOffset;

				positionBuffers.emplace_back(std::move(bufferUniqueID));

				pipelineSpecs.positionComponentType = static_cast<glTFcomponentType>(thisAccessor.componentType);
			}

			pipelineSpecs.dsg_ptr = in_dsg_ptr;
			pipelineSpecs.renderpass_ptr = in_renderpass_ptr;
			pipelineSpecs.subpassID = in_subpassID;

			auto pipelineIDIndex = pipelinesOfPrimitives.getPipelineIDIndex(pipelineSpecs , in_device_ptr);
			thisPrimitiveInfo.firstPassPipelineID = pipelinesOfPrimitives.pipelineIDs[pipelineIDIndex];
					

			primitives.emplace_back(thisPrimitiveInfo);

		}
	}
}

SceneMeshes::~SceneMeshes()
{
	indexBuffers.clear();
	positionBuffers.clear();
	normalBuffers.clear();
	tangentBuffers.clear();
	textcoord0Buffers.clear();
}

void SceneMeshes::Draw(uint32_t in_mesh, uint32_t in_meshID, Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr, Anvil::DescriptorSet* in_dsg_ptr, Anvil::BaseDevice* in_device_ptr)
{
	const MeshRange& thisMeshRange = meshes.at(in_mesh);
	auto gfx_manager_ptr(in_device_ptr->get_graphics_pipeline_manager());
 
	for (size_t primitiveIndex = thisMeshRange.primitiveFirstOffset;  primitiveIndex < thisMeshRange.primitiveFirstOffset + thisMeshRange.primitiveRangeSize; primitiveIndex++)
	{
		const PrimitiveInfo& thisPrimitive = primitives[primitiveIndex];
		in_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
													   gfx_manager_ptr->get_pipeline_layout(thisPrimitive.firstPassPipelineID),
													   0, /* in_first_set */
													   1, /* in_set_count */
													   &in_dsg_ptr,
													   0,        /* in_dynamic_offset_count */
													   nullptr); /* in_dynamic_offset_ptrs  */

		in_cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
												thisPrimitive.firstPassPipelineID);

		in_cmd_buffer_ptr->record_bind_index_buffer(indexBuffers[thisPrimitive.indexBufferIndex].get(),
													thisPrimitive.BufferOffsets.indexBufferOffset,
													thisPrimitive.indexBufferType);

		Anvil::Buffer* vertex_buffers[] =
		{
			positionBuffers[thisPrimitive.positionBufferIndex].get()
		};
		VkDeviceSize vertex_buffer_offsets[] =
		{
					thisPrimitive.BufferOffsets.positionBufferOffset
		};
		const uint32_t n_vertex_buffers = sizeof(vertex_buffers) / sizeof(vertex_buffers[0]);

		in_cmd_buffer_ptr->record_bind_vertex_buffers(0, /* in_start_binding */
												      n_vertex_buffers,
												      vertex_buffers,
												      vertex_buffer_offsets);

		in_cmd_buffer_ptr->record_draw_indexed(thisPrimitive.indicesCount,
											   1,				  /* in_instance_count */
											   0,				  /* in_first_index    */
											   0,				  /* in_vertex_offset  */
											   in_meshID);        /* in_first_instance_ID */
	}
}

Anvil::BufferUniquePtr SceneMeshes::CreateBufferForBufferViewAndCopy(tinygltf::Model& in_model, tinygltf::BufferView& in_bufferview, Anvil::BufferUsageFlagBits in_bufferusageflag, Anvil::BaseDevice* in_device_ptr)
{
	auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(in_device_ptr,
																	in_bufferview.byteLength,
																	Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
																	Anvil::SharingMode::EXCLUSIVE,
																	Anvil::BufferCreateFlagBits::NONE,
																	in_bufferusageflag);

	Anvil::BufferUniquePtr buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

	auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(in_device_ptr);

	allocator_ptr->add_buffer(buffer_ptr.get(),
							  Anvil::MemoryFeatureFlagBits::NONE);

	buffer_ptr->write(0,
					  in_bufferview.byteLength,
					  &in_model.buffers[in_bufferview.buffer].data.at(0) + in_bufferview.byteOffset);

	return std::move(buffer_ptr);
}