#include "Drawer.h"

#include <limits>
#include <cassert>

Drawer::Drawer(std::string primitives_set_sorted_by_name,
               PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
               Anvil::BaseDevice* const in_device_ptr)
    :primitivesOfMeshes_ptr(in_primitivesOfMeshes_ptr),
     primitivesSetSortedByName(primitives_set_sorted_by_name),
     device_ptr(in_device_ptr)
{
}

void Drawer::AddDrawRequests(std::vector<DrawRequest> in_draw_requests)
{
    const std::vector<PrimitiveSpecificSetInfo>& sorted_primitives_set_infos = primitivesOfMeshes_ptr->GetPrimitivesSetInfos(primitivesSetSortedByName);
    for (const auto& this_draw_request : in_draw_requests)
    {
        VkPipeline this_vk_pipeline = sorted_primitives_set_infos[this_draw_request.primitiveIndex].vkGraphicsPipeline;

        auto search = by_pipeline_VkPipelineToDrawRequests_umap.find(this_vk_pipeline);
        if (search != by_pipeline_VkPipelineToDrawRequests_umap.end())
            search->second.emplace_back(this_draw_request);
        else
            by_pipeline_VkPipelineToDrawRequests_umap.emplace(this_vk_pipeline, std::initializer_list<DrawRequest> {this_draw_request});
    }

}

void Drawer::DrawCallRequests(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                              std::string in_primitives_set_name,
                              const DescriptorSetsPtrsCollection in_descriptor_sets_ptrs_collection)
{
    CommandBufferState command_buffer_state;

    const std::vector<PrimitiveSpecificSetInfo>& primitives_set_infos = primitivesOfMeshes_ptr->GetPrimitivesSetInfos(in_primitives_set_name);

    for (const auto& this_draw_request_vector : by_pipeline_VkPipelineToDrawRequests_umap)
        for (const auto& this_draw_request : this_draw_request_vector.second)
            DrawCall(in_cmd_buffer_ptr,
                     in_descriptor_sets_ptrs_collection,
                     primitives_set_infos,
                     this_draw_request,
                     command_buffer_state);

}

void Drawer::DrawCall(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                      const DescriptorSetsPtrsCollection& in_descriptor_sets_ptrs_collection,
                      const std::vector<PrimitiveSpecificSetInfo>& in_primitives_set_infos,
                      const DrawRequest in_draw_request,
                      CommandBufferState& ref_command_buffer_state)
{

    const PrimitiveSpecificSetInfo& this_primitiveSpecificSetInfo = in_primitives_set_infos[in_draw_request.primitiveIndex];

    {
        if (this_primitiveSpecificSetInfo.vkGraphicsPipeline != ref_command_buffer_state.vkGraphicsPipeline)
        {
            in_cmd_buffer_ptr->record_bind_vk_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                       this_primitiveSpecificSetInfo.vkGraphicsPipeline);
            ref_command_buffer_state.vkGraphicsPipeline = this_primitiveSpecificSetInfo.vkGraphicsPipeline;
        }
    }

    {
        std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs;
        if (in_descriptor_sets_ptrs_collection.camera_description_set_ptr    != nullptr)
            descriptor_sets_ptrs.emplace_back(in_descriptor_sets_ptrs_collection.camera_description_set_ptr);
        if (in_descriptor_sets_ptrs_collection.camera_description_set_ptr    != nullptr && in_draw_request.isSkin == true)
            descriptor_sets_ptrs.emplace_back(in_descriptor_sets_ptrs_collection.skin_description_set_ptr);
        if (in_descriptor_sets_ptrs_collection.materials_description_set_ptr != nullptr && this_primitiveSpecificSetInfo.materialIndex != -1)
            descriptor_sets_ptrs.emplace_back(in_descriptor_sets_ptrs_collection.materials_description_set_ptr);

        if (descriptor_sets_ptrs != ref_command_buffer_state.descriptor_sets_ptrs)
        {
            const uint32_t descriptor_sets_count = static_cast<const uint32_t>(descriptor_sets_ptrs.size());
            in_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                           this_primitiveSpecificSetInfo.pipelineLayout_ptr,
                                                           0,                           /* in_first_set */
                                                           static_cast<const uint32_t>(descriptor_sets_ptrs.size()), /* in_set_count */
                                                           descriptor_sets_ptrs.data(), /* in_set_data */
                                                           0,                           /* in_dynamic_offset_count */
                                                           nullptr);                    /* in_dynamic_offset_ptrs  */

            ref_command_buffer_state.descriptor_sets_ptrs = descriptor_sets_ptrs;
        }
    }

    // TODO
    {
        if (in_draw_request.objectID != ref_command_buffer_state.objectID)
        {
            if(in_draw_request.isSkin == false)
            {
                in_cmd_buffer_ptr->record_push_constants(this_primitiveSpecificSetInfo.pipelineLayout_ptr,
                                                         Anvil::ShaderStageFlagBits::VERTEX_BIT,
                                                         0,                                            /*in_offset*/
                                                         static_cast<uint32_t>(sizeof(glm::mat4)),     /*in_size*/
                                                         &in_draw_request.vertexData);                 /*in_data*/
            }
            else
            {
                in_cmd_buffer_ptr->record_push_constants(this_primitiveSpecificSetInfo.pipelineLayout_ptr,
                                                         Anvil::ShaderStageFlagBits::VERTEX_BIT,
                                                         0,                                            /*in_offset*/
                                                         static_cast<uint32_t>(2 * sizeof(uint32_t)),  /*in_size*/
                                                         &in_draw_request.vertexData);                 /*in_data*/
            }

            if(this_primitiveSpecificSetInfo.materialIndex != -1)
            {
                struct  // 32 byte
                {
                    uint32_t materialIndex;             // 4  byte
                    MaterialMapsIndexes materialMaps;   // 24 byte
                    uint32_t alignment = 0;             // 4  byte
                } data;

                data.materialIndex = this_primitiveSpecificSetInfo.materialIndex;
                data.materialMaps = this_primitiveSpecificSetInfo.materialMaps;

                in_cmd_buffer_ptr->record_push_constants(this_primitiveSpecificSetInfo.pipelineLayout_ptr,
                                                         Anvil::ShaderStageFlagBits::FRAGMENT_BIT,
                                                         96,                                                  /*in_offset*/
                                                         static_cast<uint32_t>(sizeof(data)),                 /*in_size*/
                                                         &data);                                              /*in_data*/

                ref_command_buffer_state.objectID = in_draw_request.objectID;
            }

        }
    }


    uint32_t first_index;

    {
        uint32_t size_of_indexType;
        if (this_primitiveSpecificSetInfo.indexBufferType == Anvil::IndexType::UINT16)
            size_of_indexType = sizeof(uint16_t);
        else
            size_of_indexType = sizeof(uint32_t);

        first_index = static_cast<uint32_t>((this_primitiveSpecificSetInfo.indexBufferOffset - ref_command_buffer_state.indexBufferOffset) / size_of_indexType);
        if (this_primitiveSpecificSetInfo.indexBufferOffset < ref_command_buffer_state.indexBufferOffset || this_primitiveSpecificSetInfo.indexBufferType != ref_command_buffer_state.indexBufferType
           || (this_primitiveSpecificSetInfo.indexBufferOffset - ref_command_buffer_state.indexBufferOffset) % size_of_indexType != 0 || this_primitiveSpecificSetInfo.indexBufferOffset - ref_command_buffer_state.indexBufferOffset >(std::numeric_limits<uint32_t>::max)())
        {
            in_cmd_buffer_ptr->record_bind_index_buffer(primitivesOfMeshes_ptr->GetIndexBufferPtr(),
                                                        this_primitiveSpecificSetInfo.indexBufferOffset,
                                                        this_primitiveSpecificSetInfo.indexBufferType);

            ref_command_buffer_state.indexBufferOffset = this_primitiveSpecificSetInfo.indexBufferOffset;
            ref_command_buffer_state.indexBufferType = this_primitiveSpecificSetInfo.indexBufferType;

            first_index = 0;
        }
    }

    int32_t vertex_offset;

    {
        // checks if a suggested vertex offset can offset correctly all the buffers
        int32_t suggested_vertex_offset = static_cast<int32_t>((this_primitiveSpecificSetInfo.positionBufferOffset - ref_command_buffer_state.positionBufferOffset) / (3 * sizeof(float)));

        if (this_primitiveSpecificSetInfo.positionBufferOffset - ref_command_buffer_state.positionBufferOffset > (std::numeric_limits<int32_t>::max)()
         || this_primitiveSpecificSetInfo.positionBufferOffset < ref_command_buffer_state.positionBufferOffset
         || this_primitiveSpecificSetInfo.normalBufferOffset == -1 && ref_command_buffer_state.normalBufferOffset != -1
         || this_primitiveSpecificSetInfo.normalBufferOffset != -1 && (ref_command_buffer_state.positionBufferOffset == -1 || this_primitiveSpecificSetInfo.normalBufferOffset - ref_command_buffer_state.normalBufferOffset != 3 * static_cast<uint64_t>(suggested_vertex_offset) * sizeof(float))
         || this_primitiveSpecificSetInfo.tangentBufferOffset == -1 && ref_command_buffer_state.tangentBufferOffset != -1
         || this_primitiveSpecificSetInfo.tangentBufferOffset != -1 && (ref_command_buffer_state.tangentBufferOffset == -1 || this_primitiveSpecificSetInfo.tangentBufferOffset - ref_command_buffer_state.tangentBufferOffset != 4 * static_cast<uint64_t>(suggested_vertex_offset) * sizeof(float))
         || this_primitiveSpecificSetInfo.texcoord0BufferOffset == -1 && ref_command_buffer_state.texcoord0BufferOffset != -1
         || this_primitiveSpecificSetInfo.texcoord0BufferOffset != -1 && (ref_command_buffer_state.texcoord0BufferOffset == -1 || this_primitiveSpecificSetInfo.texcoord0BufferOffset - ref_command_buffer_state.texcoord0BufferOffset != 2 * static_cast<uint64_t>(suggested_vertex_offset) * GetSizeOfComponent(this_primitiveSpecificSetInfo.texcoord0ComponentType))
         || this_primitiveSpecificSetInfo.texcoord1BufferOffset == -1 && ref_command_buffer_state.texcoord1BufferOffset != -1
         || this_primitiveSpecificSetInfo.texcoord1BufferOffset != -1 && (ref_command_buffer_state.texcoord1BufferOffset == -1 || this_primitiveSpecificSetInfo.texcoord1BufferOffset - ref_command_buffer_state.texcoord1BufferOffset != 2 * static_cast<uint64_t>(suggested_vertex_offset) * GetSizeOfComponent(this_primitiveSpecificSetInfo.texcoord1ComponentType))
         || this_primitiveSpecificSetInfo.color0BufferOffset == -1 && ref_command_buffer_state.color0BufferOffset != -1
         || this_primitiveSpecificSetInfo.color0BufferOffset != -1 && (ref_command_buffer_state.color0BufferOffset == -1 || this_primitiveSpecificSetInfo.color0BufferOffset - ref_command_buffer_state.color0BufferOffset != 4 * static_cast<uint64_t>(suggested_vertex_offset) * GetSizeOfComponent(this_primitiveSpecificSetInfo.color0ComponentType))
         || this_primitiveSpecificSetInfo.joints0BufferOffset == -1 && ref_command_buffer_state.joints0BufferOffset != -1
         || this_primitiveSpecificSetInfo.joints0BufferOffset != -1 && (ref_command_buffer_state.joints0BufferOffset == -1 || this_primitiveSpecificSetInfo.joints0BufferOffset - ref_command_buffer_state.joints0BufferOffset != 4 * static_cast<uint64_t>(suggested_vertex_offset)* GetSizeOfComponent(this_primitiveSpecificSetInfo.joints0ComponentType))
         || this_primitiveSpecificSetInfo.weights0BufferOffset == -1 && ref_command_buffer_state.weights0BufferOffset != -1
         || this_primitiveSpecificSetInfo.weights0BufferOffset != -1 && (ref_command_buffer_state.weights0BufferOffset == -1 || this_primitiveSpecificSetInfo.weights0BufferOffset - ref_command_buffer_state.weights0BufferOffset != 4 * static_cast<uint64_t>(suggested_vertex_offset)* GetSizeOfComponent(this_primitiveSpecificSetInfo.weights0ComponentType)))
        {
            std::vector<Anvil::Buffer*> vertex_buffers;
            std::vector<VkDeviceSize> vertex_buffer_offsets;

            vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetPositionBufferPtr());
            vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.positionBufferOffset);

            ref_command_buffer_state.positionBufferOffset = this_primitiveSpecificSetInfo.positionBufferOffset;

            if (this_primitiveSpecificSetInfo.normalBufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetNormalBufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.normalBufferOffset);

                ref_command_buffer_state.normalBufferOffset = this_primitiveSpecificSetInfo.normalBufferOffset;
            }
            else
                ref_command_buffer_state.normalBufferOffset = -1;

            if (this_primitiveSpecificSetInfo.tangentBufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetTangentBufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.tangentBufferOffset);

                ref_command_buffer_state.tangentBufferOffset = this_primitiveSpecificSetInfo.tangentBufferOffset;
            }
            else
                ref_command_buffer_state.tangentBufferOffset = -1;

            if (this_primitiveSpecificSetInfo.texcoord0BufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetTexcoord0BufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.texcoord0BufferOffset);

                ref_command_buffer_state.texcoord0BufferOffset = this_primitiveSpecificSetInfo.texcoord0BufferOffset;
            }
            else
                ref_command_buffer_state.texcoord0BufferOffset = -1;

            if (this_primitiveSpecificSetInfo.texcoord1BufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetTexcoord1BufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.texcoord1BufferOffset);

                ref_command_buffer_state.texcoord1BufferOffset = this_primitiveSpecificSetInfo.texcoord1BufferOffset;
            }
            else
                ref_command_buffer_state.texcoord1BufferOffset = -1;

            if (this_primitiveSpecificSetInfo.color0BufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetColor0BufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.color0BufferOffset);

                ref_command_buffer_state.color0BufferOffset = this_primitiveSpecificSetInfo.color0BufferOffset;
            }
            else
                ref_command_buffer_state.color0BufferOffset = -1;

            if (this_primitiveSpecificSetInfo.joints0BufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetJoints0BufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.joints0BufferOffset);

                ref_command_buffer_state.joints0BufferOffset = this_primitiveSpecificSetInfo.joints0BufferOffset;
            }
            else
                ref_command_buffer_state.joints0BufferOffset = -1;

            if (this_primitiveSpecificSetInfo.weights0BufferOffset != -1)
            {
                vertex_buffers.emplace_back(primitivesOfMeshes_ptr->GetWeights0BufferPtr());
                vertex_buffer_offsets.emplace_back(this_primitiveSpecificSetInfo.weights0BufferOffset);

                ref_command_buffer_state.weights0BufferOffset = this_primitiveSpecificSetInfo.weights0BufferOffset;
            }
            else
                ref_command_buffer_state.weights0BufferOffset = -1;

            in_cmd_buffer_ptr->record_bind_vertex_buffers(0, /* in_start_binding */
                                                          static_cast<uint32_t>(vertex_buffers.size()),
                                                          vertex_buffers.data(),
                                                          vertex_buffer_offsets.data());

            vertex_offset = 0;
        }
        else
            vertex_offset = static_cast<int32_t>((this_primitiveSpecificSetInfo.positionBufferOffset - ref_command_buffer_state.positionBufferOffset) / (3 * sizeof(float)));
    }

    in_cmd_buffer_ptr->record_draw_indexed(this_primitiveSpecificSetInfo.indicesCount,
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