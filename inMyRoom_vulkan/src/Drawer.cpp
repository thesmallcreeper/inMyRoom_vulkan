#include "Drawer.h"

#include <limits>
#include <cassert>

Drawer::Drawer(sorting in_sorting_method,
               std::string only_by_pipeline_primitives_set_name,
               PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
               Anvil::BaseDevice* const in_device_ptr)
    :primitivesOfMeshes_ptr(in_primitivesOfMeshes_ptr),
     sortingMethod(in_sorting_method),
     primitivesSetNameForByPipeline(only_by_pipeline_primitives_set_name),
     device_ptr(in_device_ptr)
{
    assert(in_sorting_method == sorting::by_pipeline && !only_by_pipeline_primitives_set_name.empty());
}



void Drawer::addDrawRequests(std::vector<DrawRequest> in_draw_requests)
{
    switch (sortingMethod)
    {
        case sorting::none:
            none_drawRequests.insert(none_drawRequests.end(), in_draw_requests.begin(), in_draw_requests.end());
            break;
        case sorting::by_pipeline:
        {
            const std::vector<PrimitiveSpecificSetInfo>& primitives_set_infos = primitivesOfMeshes_ptr->getPrimitivesSetInfos(primitivesSetNameForByPipeline);
            for (const auto& this_draw_request : in_draw_requests)
            {
                VkPipeline this_vk_pipeline = primitives_set_infos[this_draw_request.primitiveIndex].vkGraphicsPipeline;

                auto search = by_pipeline_VkPipelineToDrawRequests_umap.find(this_vk_pipeline);
                if (search != by_pipeline_VkPipelineToDrawRequests_umap.end())
                    search->second.emplace_back(this_draw_request);
                else
                    by_pipeline_VkPipelineToDrawRequests_umap.emplace(this_vk_pipeline, std::initializer_list<DrawRequest> {this_draw_request});
            }
            break;
        }
        case sorting::by_increasing_z_depth:
        default:
            assert(0);
    }
}

void Drawer::drawCallRequests(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                              std::string in_primitives_set_name,
                              const std::vector<Anvil::DescriptorSet*> in_low_descriptor_sets_ptrs)
{
    CommandBufferState command_buffer_state;

    const std::vector<PrimitiveSpecificSetInfo>& primitives_set_infos = primitivesOfMeshes_ptr->getPrimitivesSetInfos(in_primitives_set_name);
    const std::vector<PrimitiveGeneralInfo>& primitives_general_infos = primitivesOfMeshes_ptr->getPrimitivesGeneralInfos();

    switch (sortingMethod)
    {
        case sorting::none:
            for (const auto& this_draw_request : none_drawRequests)
                drawCall(in_cmd_buffer_ptr,
                         in_low_descriptor_sets_ptrs,
                         primitives_set_infos,
                         primitives_general_infos,
                         this_draw_request,
                         command_buffer_state);
            break;
        case sorting::by_pipeline:
            for (const auto& this_draw_request_vector : by_pipeline_VkPipelineToDrawRequests_umap)
                for (const auto& this_draw_request : this_draw_request_vector.second)
                    drawCall(in_cmd_buffer_ptr,
                             in_low_descriptor_sets_ptrs,
                             primitives_set_infos,
                             primitives_general_infos,
                             this_draw_request,
                             command_buffer_state);
            break;
        default:
            assert(0);
    }
}

void Drawer::drawCall(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                      const std::vector<Anvil::DescriptorSet*>& in_low_descriptor_sets_ptrs,
                      const std::vector<PrimitiveSpecificSetInfo>& in_primitives_set_infos,
                      const std::vector<PrimitiveGeneralInfo>& in_primitives_general_infos,
                      const DrawRequest in_draw_request,
                      CommandBufferState& ref_command_buffer_state)
{

    const PrimitiveSpecificSetInfo& this_primitiveSpecificSetInfo = in_primitives_set_infos[in_draw_request.primitiveIndex];

    const PrimitiveGeneralInfo& this_primitiveGeneralInfo = in_primitives_general_infos[in_draw_request.primitiveIndex];

    {

        if (this_primitiveSpecificSetInfo.vkGraphicsPipeline != ref_command_buffer_state.vkGraphicsPipeline)
        {
            in_cmd_buffer_ptr->record_bind_vk_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                       this_primitiveSpecificSetInfo.vkGraphicsPipeline);
            ref_command_buffer_state.vkGraphicsPipeline = this_primitiveSpecificSetInfo.vkGraphicsPipeline;
        }
    }

    {
        std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs = in_low_descriptor_sets_ptrs;
        if (this_primitiveSpecificSetInfo.materialDescriptorSet_ptr != nullptr)
            descriptor_sets_ptrs.emplace_back(this_primitiveSpecificSetInfo.materialDescriptorSet_ptr);

        if (descriptor_sets_ptrs != ref_command_buffer_state.descriptor_sets_ptrs)
        {
            const uint32_t descriptor_sets_count = static_cast<const uint32_t>(descriptor_sets_ptrs.size());
            in_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                           this_primitiveSpecificSetInfo.pipelineLayout_ptr,
                                                           0, /* in_first_set */
                                                           descriptor_sets_ptrs.size(), /* in_set_count */
                                                           descriptor_sets_ptrs.data(),
                                                           0, /* in_dynamic_offset_count */
                                                           nullptr); /* in_dynamic_offset_ptrs  */

            ref_command_buffer_state.descriptor_sets_ptrs = descriptor_sets_ptrs;
        }
    }

    {
        if (in_draw_request.objectID != ref_command_buffer_state.objectID)
        {
            in_cmd_buffer_ptr->record_push_constants(this_primitiveSpecificSetInfo.pipelineLayout_ptr,
                                                     Anvil::ShaderStageFlagBits::VERTEX_BIT,
                                                     0,                     /*in_offset*/
                                                     sizeof(float) * 4 * 4, /*in_size*/
                                                     &in_draw_request.TRSmatrix);

            ref_command_buffer_state.objectID = in_draw_request.objectID;
        }
    }

    uint32_t first_index;

    {
        uint32_t size_of_indexType;
        if (this_primitiveGeneralInfo.indexBufferType == Anvil::IndexType::UINT16)
            size_of_indexType = sizeof(uint16_t);
        else
            size_of_indexType = sizeof(uint32_t);

        first_index = static_cast<uint32_t>((this_primitiveGeneralInfo.indexBufferOffset - ref_command_buffer_state.indexBufferOffset) / size_of_indexType);
        if (this_primitiveGeneralInfo.indexBufferOffset < ref_command_buffer_state.indexBufferOffset || this_primitiveGeneralInfo.indexBufferType != ref_command_buffer_state.indexBufferType ||
            (this_primitiveGeneralInfo.indexBufferOffset - ref_command_buffer_state.indexBufferOffset) % size_of_indexType != 0 || this_primitiveGeneralInfo.indexBufferOffset - ref_command_buffer_state.indexBufferOffset >(std::numeric_limits<uint32_t>::max)())
        {
            in_cmd_buffer_ptr->record_bind_index_buffer(primitivesOfMeshes_ptr->indexBuffer_uptr.get(),
                                                        this_primitiveGeneralInfo.indexBufferOffset,
                                                        this_primitiveGeneralInfo.indexBufferType);

            ref_command_buffer_state.indexBufferOffset = this_primitiveGeneralInfo.indexBufferOffset;
            ref_command_buffer_state.indexBufferType = this_primitiveGeneralInfo.indexBufferType;

            first_index = 0;
        }
    }

    int32_t vertex_offset;

    {
        // checks if a suggested vertex offset can offset correctly all the buffers
        int32_t suggested_vertex_offset = static_cast<int32_t>((this_primitiveGeneralInfo.positionBufferOffset - ref_command_buffer_state.positionBufferOffset) / (3 * sizeof(float)));

        if (this_primitiveGeneralInfo.positionBufferOffset - ref_command_buffer_state.positionBufferOffset > (std::numeric_limits<int32_t>::max)()
         || this_primitiveGeneralInfo.positionBufferOffset < ref_command_buffer_state.positionBufferOffset
         || this_primitiveSpecificSetInfo.usesNormalBuffer == false && ref_command_buffer_state.normalBufferOffset != -1
         || this_primitiveSpecificSetInfo.usesNormalBuffer == true && (ref_command_buffer_state.positionBufferOffset == -1 || this_primitiveGeneralInfo.normalBufferOffset - ref_command_buffer_state.normalBufferOffset != 3 * static_cast<uint64_t>(suggested_vertex_offset) * sizeof(float))
         || this_primitiveSpecificSetInfo.usesTangentBuffer == false && ref_command_buffer_state.tangentBufferOffset != -1
         || this_primitiveSpecificSetInfo.usesTangentBuffer == true && (ref_command_buffer_state.tangentBufferOffset == -1 || this_primitiveGeneralInfo.tangentBufferOffset - ref_command_buffer_state.tangentBufferOffset != 4 * static_cast<uint64_t>(suggested_vertex_offset) * sizeof(float))
         || this_primitiveSpecificSetInfo.usesTexcoord0Buffer == false && ref_command_buffer_state.texcoord0BufferOffset != -1
         || this_primitiveSpecificSetInfo.usesTexcoord0Buffer == true && (ref_command_buffer_state.texcoord0BufferOffset == -1 || this_primitiveGeneralInfo.texcoord0BufferOffset - ref_command_buffer_state.texcoord0BufferOffset != 2 * static_cast<uint64_t>(suggested_vertex_offset) * GetSizeOfComponent(this_primitiveGeneralInfo.pipelineSpecs.texcoord0ComponentType))
         || this_primitiveSpecificSetInfo.usesTexcoord1Buffer == false && ref_command_buffer_state.texcoord1BufferOffset != -1
         || this_primitiveSpecificSetInfo.usesTexcoord1Buffer == true && (ref_command_buffer_state.texcoord1BufferOffset == -1 || this_primitiveGeneralInfo.texcoord1BufferOffset - ref_command_buffer_state.texcoord1BufferOffset != 2 * static_cast<uint64_t>(suggested_vertex_offset) * GetSizeOfComponent(this_primitiveGeneralInfo.pipelineSpecs.texcoord1ComponentType))
         || this_primitiveSpecificSetInfo.usesColor0Buffer == false && ref_command_buffer_state.color0BufferOffset != -1
         || this_primitiveSpecificSetInfo.usesColor0Buffer == true && (ref_command_buffer_state.color0BufferOffset == -1 || this_primitiveGeneralInfo.color0BufferOffset - ref_command_buffer_state.color0BufferOffset != 4 * static_cast<uint64_t>(suggested_vertex_offset) * GetSizeOfComponent(this_primitiveGeneralInfo.pipelineSpecs.color0ComponentType)))
        {
            std::vector<Anvil::Buffer*> vertex_buffers;
            std::vector<VkDeviceSize> vertex_buffer_offsets;

            vertex_buffers.emplace_back(primitivesOfMeshes_ptr->positionBuffer_uptr.get());
            vertex_buffer_offsets.emplace_back(this_primitiveGeneralInfo.positionBufferOffset);

            ref_command_buffer_state.positionBufferOffset = this_primitiveGeneralInfo.positionBufferOffset;

            if (this_primitiveSpecificSetInfo.usesNormalBuffer == true)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->normalBuffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitiveGeneralInfo.normalBufferOffset);

                ref_command_buffer_state.normalBufferOffset = this_primitiveGeneralInfo.normalBufferOffset;
            }
            else
                ref_command_buffer_state.normalBufferOffset = -1;

            if (this_primitiveSpecificSetInfo.usesTangentBuffer == true)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->tangentBuffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitiveGeneralInfo.tangentBufferOffset);

                ref_command_buffer_state.tangentBufferOffset = this_primitiveGeneralInfo.tangentBufferOffset;
            }
            else
                ref_command_buffer_state.tangentBufferOffset = -1;

            if (this_primitiveSpecificSetInfo.usesTexcoord0Buffer == true)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->texcoord0Buffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitiveGeneralInfo.texcoord0BufferOffset);

                ref_command_buffer_state.texcoord0BufferOffset = this_primitiveGeneralInfo.texcoord0BufferOffset;
            }
            else
                ref_command_buffer_state.texcoord0BufferOffset = -1;

            if (this_primitiveSpecificSetInfo.usesTexcoord1Buffer == true)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->texcoord1Buffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitiveGeneralInfo.texcoord1BufferOffset);

                ref_command_buffer_state.texcoord1BufferOffset = this_primitiveGeneralInfo.texcoord1BufferOffset;
            }
            else
                ref_command_buffer_state.texcoord1BufferOffset = -1;

            if (this_primitiveSpecificSetInfo.usesColor0Buffer == true)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->color0Buffer_uptr.get());
                vertex_buffer_offsets.emplace_back(this_primitiveGeneralInfo.color0BufferOffset);

                ref_command_buffer_state.color0BufferOffset = this_primitiveGeneralInfo.color0BufferOffset;
            }
            else
                ref_command_buffer_state.color0BufferOffset = -1;


            in_cmd_buffer_ptr->record_bind_vertex_buffers(0, /* in_start_binding */
                                                          static_cast<uint32_t>(vertex_buffers.size()),
                                                          vertex_buffers.data(),
                                                          vertex_buffer_offsets.data());

            vertex_offset = 0;
        }
        else
            vertex_offset = static_cast<int32_t>((this_primitiveGeneralInfo.positionBufferOffset - ref_command_buffer_state.positionBufferOffset) / (3 * sizeof(float)));
    }

    in_cmd_buffer_ptr->record_draw_indexed(this_primitiveGeneralInfo.indicesCount,
                                           1, /* in_instance_count */
                                           first_index, /* in_first_index    */
                                           vertex_offset, /* in_vertex_offset  */
                                           in_draw_request.objectID); /* in_first_instance_ID */
}

size_t Drawer::GetSizeOfComponent(glTFcomponentType component_type)
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