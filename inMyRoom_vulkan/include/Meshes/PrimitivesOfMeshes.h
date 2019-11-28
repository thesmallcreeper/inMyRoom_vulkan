#pragma once

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/base_pipeline_manager.h"
#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "tiny_gltf.h"

#include "Geometry/OBB.h"

#include "PipelinesFactory.h"
#include "ShadersSetsFamiliesCache.h"
#include "Meshes/MaterialsOfPrimitives.h"

struct PrimitiveGeneralInfo
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
    GraphicsPipelineSpecs pipelineSpecs;
    size_t materialIndex = -1;
};

struct PrimitiveSpecificSetInfo
{
    bool usesNormalBuffer = false;
    bool usesTangentBuffer = false;
    bool usesTexcoord0Buffer = false;
    bool usesTexcoord1Buffer = false;
    bool usesColor0Buffer = false;
    Anvil::DescriptorSet* materialDescriptorSet_ptr = nullptr;
    VkPipeline vkGraphicsPipeline;
    Anvil::PipelineLayout* pipelineLayout_ptr;
};

struct PrimitivesSetSpecs
{
    std::string primitivesSetName;
    ShadersSpecs shaderSpecs;
    bool useMaterial;
    bool useDepthWrite;
    Anvil::CompareOp depthCompare;
};

class PrimitivesOfMeshes
{
public: // functions
    PrimitivesOfMeshes(PipelinesFactory* in_pipelinesFactory_ptr,
                       ShadersSetsFamiliesCache* in_shadersSetsFamiliesCache_ptr,
                       MaterialsOfPrimitives* in_materialsOfPrimitives_ptr,
                       Anvil::BaseDevice* const in_device_ptr);

    ~PrimitivesOfMeshes();

    void AddPrimitive(const tinygltf::Model& in_model,
                      const tinygltf::Primitive& in_primitive);

    void InitPrimitivesSet(PrimitivesSetSpecs in_primitives_set_specs,
                           const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_lower_descriptorSetCreateInfos,
                           Anvil::RenderPass* renderpass_ptr,
                           Anvil::SubPassID subpassID);

    void FlashDevice();

    size_t GetPrimitivesCount();

    void StartRecordOBB();

    OBB  GetOBBandReset();

    const std::vector<PrimitiveSpecificSetInfo>& GetPrimitivesSetInfos(std::string in_primitives_set_name) const;
    const std::vector<PrimitiveGeneralInfo>& GetPrimitivesGeneralInfos() const;

    Anvil::Buffer* GetIndexBufferPtr() { return indexBuffer_uptr.get(); }
    Anvil::Buffer* GetPositionBufferPtr() { return positionBuffer_uptr.get(); }
    Anvil::Buffer* GetNormalBufferPtr() { return normalBuffer_uptr.get(); }
    Anvil::Buffer* GetTangentBufferPtr() { return tangentBuffer_uptr.get(); }
    Anvil::Buffer* GetTexcoord0BufferPtr() { return texcoord0Buffer_uptr.get(); }
    Anvil::Buffer* GetTexcoord1BufferPtr() { return texcoord1Buffer_uptr.get(); }
    Anvil::Buffer* GetColor0BufferPtr() { return color0Buffer_uptr.get(); }

private: // functions
    void AddAccessorDataToLocalBuffer(std::vector<unsigned char>& localBuffer_ref,
                                      bool shouldFlipYZ_position,
                                      bool vec3_to_vec4,
                                      size_t alignment,
                                      const tinygltf::Model& in_model,
                                      const tinygltf::Accessor& in_accessor) const;

    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                            Anvil::BufferUsageFlagBits in_bufferusageflag,
                                                            std::string buffers_name) const;
private: // data
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

    std::vector<glm::vec3> pointsOfRecordingOBB;
    bool recordingOBB = false;

    std::unordered_map<std::string, std::vector<PrimitiveSpecificSetInfo>> primitivesSetsNameToVector_umap;
    std::vector<PrimitiveGeneralInfo> primitivesGeneralInfos;

    PipelinesFactory* pipelinesFactory_ptr;
    ShadersSetsFamiliesCache* shadersSetsFamiliesCache_ptr;
    MaterialsOfPrimitives* materialsOfPrimitives_ptr;

    Anvil::BaseDevice* const device_ptr;
    bool hasBuffersBeenFlashed = false;

    Anvil::MemoryAllocatorUniquePtr allocator_ptr;
};

