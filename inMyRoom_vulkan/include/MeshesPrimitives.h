#pragma once

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "tiny_gltf.h"

#include "PrimitivesPipelines.h"
#include "PrimitivesMaterials.h"
#include "PrimitivesShaders.h"

struct PrimitiveInitInfo
{
    Anvil::IndexType indexBufferType;
    uint32_t indicesCount = 0;
    VkDeviceSize indexBufferOffset = -1;
    VkDeviceSize positionBufferOffset = -1;
    VkDeviceSize normalBufferOffset = -1;
    VkDeviceSize tangentBufferOffset = -1;
    VkDeviceSize texcoord0BufferOffset = -1;
    VkDeviceSize texcoord1BufferOffset = -1;
    VkDeviceSize color0BufferOffset = -1;
    PipelineSpecs pipelineSpecs;
    size_t materialIndex = -1;
};

struct PrimitiveInfo
{
    Anvil::IndexType indexBufferType;
    uint32_t indicesCount = 0;
    VkDeviceSize indexBufferOffset = -1;
    VkDeviceSize positionBufferOffset = -1;
    VkDeviceSize normalBufferOffset = -1;
    VkDeviceSize tangentBufferOffset = -1;
    VkDeviceSize texcoord0BufferOffset = -1;
    VkDeviceSize texcoord1BufferOffset = -1;
    VkDeviceSize color0BufferOffset = -1;
    Anvil::PipelineID thisPipelineID;
    Anvil::DescriptorSet* materialDescriptorSet_ptr = nullptr;
    Anvil::PipelineLayout* pipelineLayout_ptr = nullptr;
};

class MeshesPrimitives
{
public:
    MeshesPrimitives(PrimitivesPipelines* in_primitivesPipelines_ptr, PrimitivesShaders* in_primitivesShaders_ptr,
                     PrimitivesMaterials* in_primitivesMaterials_ptr,
                     Anvil::BaseDevice* const in_device_ptr);
    ~MeshesPrimitives();

    void AddPrimitive(tinygltf::Model& in_model, tinygltf::Primitive& in_primitive);
    void FlashBuffersToDevice();
    size_t InitPrimitivesSet(ShadersSpecs in_shader_specs, bool use_material, Anvil::CompareOp in_depth_compare, bool use_depth_write,
                             const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_lower_descriptorSetCreateInfos,
                             Anvil::RenderPass* renderpass_ptr, Anvil::SubPassID subpassID);

public:
    std::vector<std::vector<PrimitiveInfo>> primitivesSets;
    std::vector<PrimitiveInitInfo> primitivesInitInfos;

    Anvil::BufferUniquePtr indexBuffer_uptr;
    Anvil::BufferUniquePtr positionBuffer_uptr;
    Anvil::BufferUniquePtr normalBuffer_uptr;
    Anvil::BufferUniquePtr tangentBuffer_uptr;
    Anvil::BufferUniquePtr texcoord0Buffer_uptr;
    Anvil::BufferUniquePtr texcoord1Buffer_uptr;
    Anvil::BufferUniquePtr color0Buffer_uptr;

    std::vector<unsigned char> localIndexBuffer;
    std::vector<unsigned char> localPositionBuffer;
    std::vector<unsigned char> localNormalBuffer;
    std::vector<unsigned char> localTangentBuffer;
    std::vector<unsigned char> localTexcoord0Buffer;
    std::vector<unsigned char> localTexcoord1Buffer;
    std::vector<unsigned char> localColor0Buffer;

    bool hasBuffersBeenFlashed = false;

private:
    void AddAccessorDataToLocalBuffer(std::vector<unsigned char>& localBuffer_ref, bool shouldFlipYZ_position, bool vec3_to_vec4, size_t allignBufferSize,
                                      tinygltf::Model& in_model,tinygltf::Accessor in_accessor) const;
    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                            Anvil::BufferUsageFlagBits in_bufferusageflag, std::string buffers_name) const;
private:
    Anvil::BaseDevice* const device_ptr;

    PrimitivesPipelines* primitivesPipelines_ptr;
    PrimitivesShaders* primitivesShaders_ptr;
    PrimitivesMaterials* primitivesMaterials_ptr;

    Anvil::MemoryAllocatorUniquePtr allocator_ptr;
};

