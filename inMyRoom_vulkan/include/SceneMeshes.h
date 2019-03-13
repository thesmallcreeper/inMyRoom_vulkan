#pragma once

#include <cassert>

#include "tiny_gltf.h"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/graphics_pipeline_manager.h"
#include "wrappers/command_buffer.h"

#include "PrimitivesPipelines.h"

struct PrimitiveInfo
{
    Anvil::IndexType indexBufferType;
    uint32_t indicesCount = 0;
    int32_t indexBufferIndex = -1;
    int32_t positionBufferIndex = -1;
    int32_t normalBufferIndex = -1;
    int32_t tangentBufferIndex = -1;
    int32_t textcoord0BufferIndex = -1;
    struct
    {
        VkDeviceSize indexBufferOffset;
        VkDeviceSize positionBufferOffset;
        VkDeviceSize normalBufferOffset;
        VkDeviceSize tangentBufferOffset;
        VkDeviceSize textcoord0BufferOffset;
    } BufferOffsets;
    Anvil::PipelineID firstPassPipelineID;
};

struct MeshRange
{
    size_t primitiveFirstOffset;
    size_t primitiveRangeSize;
};

class SceneMeshes
{
public:
    SceneMeshes(tinygltf::Model& in_model, PrimitivesPipelines& in_pipelinesOfPrimitives,
                Anvil::DescriptorSetGroup* in_dsg_ptr, Anvil::RenderPass* in_renderpass_ptr, Anvil::SubPassID in_subpassID, Anvil::BaseDevice* in_device_ptr);
    ~SceneMeshes();

    void Draw(uint32_t in_mesh, uint32_t in_meshID, Anvil::PrimaryCommandBuffer* in_command_buffer, Anvil::DescriptorSet* in_dsg_ptr, Anvil::BaseDevice* in_device_ptr);

private:
    Anvil::BufferUniquePtr CreateBufferForBufferViewAndCopy(tinygltf::Model& in_model, tinygltf::BufferView& in_bufferview, Anvil::BufferUsageFlagBits in_bufferusageflag, Anvil::BaseDevice* in_device_ptr);

    Anvil::MemoryAllocatorUniquePtr   allocator_ptr;

    std::vector<Anvil::BufferUniquePtr> indexBuffers;
    std::vector<Anvil::BufferUniquePtr> positionBuffers;
    std::vector<Anvil::BufferUniquePtr> normalBuffers;
    std::vector<Anvil::BufferUniquePtr> tangentBuffers;
    std::vector<Anvil::BufferUniquePtr> textcoord0Buffers;

    std::vector<PrimitiveInfo> primitives;
    std::vector<MeshRange>     meshes;

    PrimitivesPipelines& pipelinesOfPrimitives;
};