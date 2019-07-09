#include "Drawer.h"

#include <limits>
#include <cassert>

Drawer::Drawer(sorting in_sorting_method, size_t in_pipelineSetIndex, MeshesPrimitives* in_meshPrimitives_ptr, Anvil::BaseDevice* const in_device_ptr)
    :meshesPrimitives_ptr(in_meshPrimitives_ptr),
     sortingMethod(in_sorting_method),
     pipelineSetIndex(in_pipelineSetIndex),
     device_ptr(in_device_ptr)
{
}

Drawer::~Drawer()
{
}


void Drawer::AddDrawRequests(std::vector<DrawRequest> in_draw_requests)
{
    switch (sortingMethod)
    {
        case sorting::none:
            none_drawRequests.insert(none_drawRequests.end(), in_draw_requests.begin(), in_draw_requests.end());
            break;
        case sorting::by_pipeline:
            for(const auto& this_draw_request: in_draw_requests)
            {
                Anvil::PipelineID& this_pipelineID = meshesPrimitives_ptr->primitivesSets[pipelineSetIndex][this_draw_request.primitive_index].thisPipelineID;
                auto search = by_pipeline_PipelineIDToDrawRequests_umap.find(this_pipelineID);
                if (search != by_pipeline_PipelineIDToDrawRequests_umap.end())
                    search->second.emplace_back(this_draw_request);
                else
                    by_pipeline_PipelineIDToDrawRequests_umap.emplace(this_pipelineID, std::initializer_list<DrawRequest> {this_draw_request});
            }
            break;
        default:
            assert(0);
    }
}

void Drawer::DrawCallRequests(std::vector<std::pair<Anvil::PrimaryCommandBuffer*, size_t>> pairs_primaryCommandBuffers_primitiveSets, const std::vector<Anvil::DescriptorSet*>& in_low_descriptor_sets_ptrs) const
{
    std::vector<CommandBufferState> command_buffer_states(pairs_primaryCommandBuffers_primitiveSets.size());

    switch (sortingMethod)
    {
        case sorting::none:
            for (const auto& this_draw_request : none_drawRequests)
                for (size_t pair_index = 0; pair_index < pairs_primaryCommandBuffers_primitiveSets.size(); pair_index++)
                    DrawCall(command_buffer_states[pair_index], pairs_primaryCommandBuffers_primitiveSets[pair_index].first, pairs_primaryCommandBuffers_primitiveSets[pair_index].second,
                             this_draw_request, in_low_descriptor_sets_ptrs);
            break;
        case sorting::by_pipeline:
            for (const auto& this_draw_request_vector : by_pipeline_PipelineIDToDrawRequests_umap)
                for (const auto& this_draw_request : this_draw_request_vector.second)
                    for (size_t pair_index = 0; pair_index < pairs_primaryCommandBuffers_primitiveSets.size(); pair_index++)
                        DrawCall(command_buffer_states[pair_index], pairs_primaryCommandBuffers_primitiveSets[pair_index].first, pairs_primaryCommandBuffers_primitiveSets[pair_index].second,
                                 this_draw_request, in_low_descriptor_sets_ptrs);
            break;
        default:
            assert(0);
    }
}

void Drawer::DeleteDrawRequests()
{
    switch (sortingMethod)
    {
        case sorting::none:
            none_drawRequests.clear();
            break;
        case sorting::by_pipeline:
            by_pipeline_PipelineIDToDrawRequests_umap.clear();
            break;
        default:
            assert(0);
    }
}

void Drawer::DrawCall(CommandBufferState& command_buffer_state, Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr, size_t in_primitivesSet_index, const DrawRequest draw_request,
                      const std::vector<Anvil::DescriptorSet*>& in_low_descriptor_sets_ptrs) const
{
    const std::vector<PrimitiveInfo>& this_primitivesSet = meshesPrimitives_ptr->primitivesSets[in_primitivesSet_index];
    const PrimitiveInfo& this_primitive = this_primitivesSet[draw_request.primitive_index];
    const PrimitiveInitInfo& this_primitiveInitInfo = meshesPrimitives_ptr->primitivesInitInfos[draw_request.primitive_index];

    uint32_t first_index;
    int32_t vertex_offset;

    {
        auto gfx_manager_ptr(device_ptr->get_graphics_pipeline_manager());

        if (this_primitive.thisPipelineID != command_buffer_state.pipeline_id)
        {
            in_cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                    this_primitive.thisPipelineID);
            command_buffer_state.pipeline_id = this_primitive.thisPipelineID;
        }
    }

    {
        std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs = in_low_descriptor_sets_ptrs;
        if (this_primitive.materialDescriptorSet_ptr != nullptr)
            descriptor_sets_ptrs.emplace_back(this_primitive.materialDescriptorSet_ptr);

        if (descriptor_sets_ptrs != command_buffer_state.descriptor_sets_ptrs)
        {
            const uint32_t descriptor_sets_count = static_cast<const uint32_t>(descriptor_sets_ptrs.size());
            in_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                           this_primitive.pipelineLayout_ptr,
                                                           0, /* in_first_set */
                                                           descriptor_sets_ptrs.size(), /* in_set_count */
                                                           descriptor_sets_ptrs.data(),
                                                           0, /* in_dynamic_offset_count */
                                                           nullptr); /* in_dynamic_offset_ptrs  */

            command_buffer_state.descriptor_sets_ptrs = descriptor_sets_ptrs;
        }
    }

    {
        uint32_t size_of_indexType;
        if (this_primitive.indexBufferType == Anvil::IndexType::UINT16)
            size_of_indexType = sizeof(uint16_t);
        else
            size_of_indexType = sizeof(uint32_t);

        first_index = static_cast<uint32_t>((this_primitive.indexBufferOffset - command_buffer_state.indexBufferOffset) / size_of_indexType);
        if (this_primitive.indexBufferOffset < command_buffer_state.indexBufferOffset || this_primitive.indexBufferType != command_buffer_state.indexBufferType ||
            (this_primitive.indexBufferOffset - command_buffer_state.indexBufferOffset) % size_of_indexType != 0 || this_primitive.indexBufferOffset - command_buffer_state.indexBufferOffset >(std::numeric_limits<uint32_t>::max)())
        {
            in_cmd_buffer_ptr->record_bind_index_buffer(meshesPrimitives_ptr->indexBuffer_uptr.get(),
                                                        this_primitive.indexBufferOffset,
                                                        this_primitive.indexBufferType);

            command_buffer_state.indexBufferOffset = this_primitive.indexBufferOffset;
            command_buffer_state.indexBufferType = this_primitive.indexBufferType;

            first_index = 0;
        }
    }

    {
        vertex_offset = static_cast<int32_t>((this_primitive.positionBufferOffset - command_buffer_state.positionBufferOffset) / (3*sizeof(float)));

        if (this_primitive.positionBufferOffset - command_buffer_state.positionBufferOffset > (std::numeric_limits<int32_t>::max)() || this_primitive.positionBufferOffset < command_buffer_state.positionBufferOffset
            || command_buffer_state.normalBufferOffset != -1 && this_primitive.normalBufferOffset ==- 1 || this_primitive.normalBufferOffset != -1
             && (this_primitive.normalBufferOffset - command_buffer_state.normalBufferOffset != 3 * static_cast<uint64_t>(vertex_offset) * sizeof(float) || command_buffer_state.positionBufferOffset == -1)
            || command_buffer_state.tangentBufferOffset != -1 && this_primitive.tangentBufferOffset == -1 || this_primitive.tangentBufferOffset != -1
             && (this_primitive.tangentBufferOffset - command_buffer_state.tangentBufferOffset != 4 * static_cast<uint64_t>(vertex_offset) * sizeof(float) || command_buffer_state.tangentBufferOffset == -1)
            || command_buffer_state.texcoord0BufferOffset != -1 && this_primitive.texcoord0BufferOffset == -1 || this_primitive.texcoord0BufferOffset != -1
             && (this_primitive.texcoord0BufferOffset - command_buffer_state.texcoord0BufferOffset != 2 * static_cast<uint64_t>(vertex_offset) * GetSizeOfComponent(this_primitiveInitInfo.pipelineSpecs.texcoord0ComponentType) || command_buffer_state.texcoord0BufferOffset == -1)
            || command_buffer_state.texcoord1BufferOffset != -1 && this_primitive.texcoord1BufferOffset == -1 || this_primitive.texcoord1BufferOffset != -1
             && (this_primitive.texcoord1BufferOffset - command_buffer_state.texcoord1BufferOffset != 2 * static_cast<uint64_t>(vertex_offset) * GetSizeOfComponent(this_primitiveInitInfo.pipelineSpecs.texcoord1ComponentType) || command_buffer_state.texcoord1BufferOffset == -1)
            || command_buffer_state.color0BufferOffset != -1 && this_primitive.color0BufferOffset == -1 || this_primitive.color0BufferOffset != -1
             && (this_primitive.color0BufferOffset - command_buffer_state.color0BufferOffset != 2 * static_cast<uint64_t>(vertex_offset) * GetSizeOfComponent(this_primitiveInitInfo.pipelineSpecs.color0ComponentType) || command_buffer_state.color0BufferOffset == -1))
        {
            std::vector<Anvil::Buffer*> vertex_buffers;
            std::vector<VkDeviceSize> vertex_buffer_offsets;

            vertex_buffers.emplace_back(meshesPrimitives_ptr->positionBuffer_uptr.get());
            vertex_buffer_offsets.emplace_back(this_primitive.positionBufferOffset);

            command_buffer_state.positionBufferOffset = this_primitive.positionBufferOffset;

            if (this_primitive.normalBufferOffset != -1)
            {
                vertex_buffers.emplace_back(meshesPrimitives_ptr->normalBuffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitive.normalBufferOffset);

                command_buffer_state.normalBufferOffset = this_primitive.normalBufferOffset;
            }
            else
                command_buffer_state.normalBufferOffset = -1;

            if (this_primitive.tangentBufferOffset != -1)
            {
                vertex_buffers.emplace_back(meshesPrimitives_ptr->tangentBuffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitive.tangentBufferOffset);

                command_buffer_state.tangentBufferOffset = this_primitive.tangentBufferOffset;
            }
            else
                command_buffer_state.tangentBufferOffset = -1;

            if (this_primitive.texcoord0BufferOffset != -1)
            {
                vertex_buffers.emplace_back(meshesPrimitives_ptr->texcoord0Buffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitive.texcoord0BufferOffset);

                command_buffer_state.texcoord0BufferOffset = this_primitive.texcoord0BufferOffset;
            }
            else
                command_buffer_state.texcoord0BufferOffset = -1;

            if (this_primitive.texcoord1BufferOffset != -1)
            {
                vertex_buffers.emplace_back(meshesPrimitives_ptr->texcoord1Buffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitive.texcoord1BufferOffset);

                command_buffer_state.texcoord1BufferOffset = this_primitive.texcoord1BufferOffset;
            }
            else
                command_buffer_state.texcoord1BufferOffset = -1;

            if (this_primitive.color0BufferOffset != -1)
            {
                vertex_buffers.emplace_back(meshesPrimitives_ptr->color0Buffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitive.color0BufferOffset);

                command_buffer_state.color0BufferOffset = this_primitive.color0BufferOffset;
            }
            else
                command_buffer_state.color0BufferOffset = -1;

            vertex_offset = 0;

            in_cmd_buffer_ptr->record_bind_vertex_buffers(0, /* in_start_binding */
                                                          static_cast<uint32_t>(vertex_buffers.size()),
                                                          vertex_buffers.data(),
                                                          vertex_buffer_offsets.data());
        }
    }

    in_cmd_buffer_ptr->record_draw_indexed(this_primitive.indicesCount,
                                           1, /* in_instance_count */
                                           first_index, /* in_first_index    */
                                           vertex_offset, /* in_vertex_offset  */
                                           draw_request.meshID); /* in_first_instance_ID */
}

size_t Drawer::GetSizeOfComponent(glTFcomponentType component_type) const
{
    size_t size_of_each_component_in_byte;
    switch (static_cast<glTFcomponentType>(component_type))
    {
        default:
        case glTFcomponentType::type_byte:
        case glTFcomponentType::type_unsigned_byte:
            size_of_each_component_in_byte = sizeof(int8_t);
            break;
        case glTFcomponentType::type_short:
        case glTFcomponentType::type_unsigned_short:
            size_of_each_component_in_byte = sizeof(int16_t);
            break;
        case glTFcomponentType::type_int:
        case glTFcomponentType::type_unsigned_int:
        case glTFcomponentType::type_float:
            size_of_each_component_in_byte = sizeof(int32_t);
            break;
        case glTFcomponentType::type_double:
            size_of_each_component_in_byte = sizeof(int64_t);
            break;
    }

    return size_of_each_component_in_byte;
}