#include "NodesMeshes.h"

#include "MeshesPrimitives.h"

NodesMeshes::NodesMeshes(tinygltf::Model& in_model, MeshesPrimitives* in_meshesPrimitives_ptr, Anvil::BaseDevice* in_device_ptr)
    :
    meshesPrimitives_ptr(in_meshesPrimitives_ptr),
    device_ptr(in_device_ptr)
{
    size_t primitives_so_far = 0;
    for(tinygltf::Mesh& this_mesh: in_model.meshes)
    {
        MeshRange this_mesh_range;
        this_mesh_range.primitiveFirstOffset = primitives_so_far;
        this_mesh_range.primitiveRangeSize = this_mesh.primitives.size();
        for (tinygltf::Primitive this_primitive : this_mesh.primitives)
            meshesPrimitives_ptr->AddPrimitive(in_model, this_primitive);

        meshes.emplace_back(this_mesh_range);
        primitives_so_far += this_mesh.primitives.size();
    }
}

NodesMeshes::~NodesMeshes()
{
    
}


void NodesMeshes::Draw(size_t in_mesh, uint32_t in_meshDeviceID, size_t in_primitivesSet_index, Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr, std::vector<Anvil::DescriptorSet*> in_low_descriptor_sets_ptrs)
{
    const MeshRange& this_meshRange = meshes[in_mesh];
    const std::vector<PrimitiveInfo>& this_primitivesSet = meshesPrimitives_ptr->primitivesSets[in_primitivesSet_index];

    auto gfx_manager_ptr(device_ptr->get_graphics_pipeline_manager());

    for (size_t primitiveIndex = this_meshRange.primitiveFirstOffset; primitiveIndex < this_meshRange.primitiveFirstOffset + this_meshRange.primitiveRangeSize; primitiveIndex++)
    {
        const PrimitiveInfo& this_primitive = this_primitivesSet[primitiveIndex];

        std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs = in_low_descriptor_sets_ptrs;
        if (this_primitive.material_descriptorSet_ptr != nullptr)
            descriptor_sets_ptrs.emplace_back(this_primitive.material_descriptorSet_ptr);

        const uint32_t descriptor_sets_count = descriptor_sets_ptrs.size();


        in_cmd_buffer_ptr->record_bind_pipeline(Anvil::PipelineBindPoint::GRAPHICS,
                                                this_primitive.thisPipelineID);

        in_cmd_buffer_ptr->record_bind_descriptor_sets(Anvil::PipelineBindPoint::GRAPHICS,
                                                       gfx_manager_ptr->get_pipeline_layout(this_primitive.thisPipelineID),
                                                       0, /* in_first_set */
                                                       descriptor_sets_count, /* in_set_count */
                                                       descriptor_sets_ptrs.data(),
                                                       0,        /* in_dynamic_offset_count */
                                                       nullptr); /* in_dynamic_offset_ptrs  */

        in_cmd_buffer_ptr->record_bind_index_buffer(meshesPrimitives_ptr->indexBuffer.get(),
                                                    this_primitive.indexBufferOffset,
                                                    this_primitive.indexBufferType);

        std::vector<Anvil::Buffer*> vertex_buffers;
        std::vector<VkDeviceSize> vertex_buffer_offsets;

        vertex_buffers.emplace_back(meshesPrimitives_ptr->positionBuffer.get());
        vertex_buffer_offsets.emplace_back(this_primitive.positionBufferOffset);

        if(this_primitive.normalBufferOffset != -1)
        {
            vertex_buffers.emplace_back(meshesPrimitives_ptr->normalBuffer.get());
            vertex_buffer_offsets.emplace_back(this_primitive.normalBufferOffset);
        }
        if (this_primitive.tangentBufferOffset != -1)
        {
            vertex_buffers.emplace_back(meshesPrimitives_ptr->tangentBuffer.get());
            vertex_buffer_offsets.emplace_back(this_primitive.tangentBufferOffset);
        }
        if (this_primitive.texcoord0BufferOffset != -1)
        {
            vertex_buffers.emplace_back(meshesPrimitives_ptr->texcoord0Buffer.get());
            vertex_buffer_offsets.emplace_back(this_primitive.texcoord0BufferOffset);
        }
        if (this_primitive.texcoord1BufferOffset != -1)
        {
            vertex_buffers.emplace_back(meshesPrimitives_ptr->texcoord1Buffer.get());
            vertex_buffer_offsets.emplace_back(this_primitive.texcoord1BufferOffset);
        }

        in_cmd_buffer_ptr->record_bind_vertex_buffers(0, /* in_start_binding */
                                                      vertex_buffers.size(),
                                                      vertex_buffers.data(),
                                                      vertex_buffer_offsets.data());

        in_cmd_buffer_ptr->record_draw_indexed(this_primitive.indicesCount,
                                               1,                 /* in_instance_count */
                                               0,                 /* in_first_index    */
                                               0,                 /* in_vertex_offset  */
                                               in_meshDeviceID);  /* in_first_instance_ID */
    }
}