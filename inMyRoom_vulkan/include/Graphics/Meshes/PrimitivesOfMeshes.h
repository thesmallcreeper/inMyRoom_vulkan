#pragma once

#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "tiny_gltf.h"

#include "Geometry/OBBtree.h"
#include "Geometry/Triangle.h"

#include "Graphics/PipelinesFactory.h"
#include "Graphics/ShadersSetsFamiliesCache.h"
#include "Graphics/Meshes/MaterialsOfPrimitives.h"

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
    VkDeviceSize joints0BufferOffset = -1;
    VkDeviceSize weights0BufferOffset = -1;

    GraphicsPipelineSpecs commonGraphicsPipelineSpecs;

    uint32_t materialIndex = -1;
    MaterialMapsIndexes materialMaps;
};

struct PrimitiveCPUdata
{
    std::vector<glm::vec3> points;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
    glTFmode drawMode;
    bool isSkin = false;
};

struct PrimitiveSpecificSetInfo
{
    Anvil::IndexType indexBufferType;
    uint32_t indicesCount = 0;

    VkDeviceSize indexBufferOffset = -1;
    VkDeviceSize positionBufferOffset = -1;
    VkDeviceSize normalBufferOffset = -1;
    VkDeviceSize tangentBufferOffset = -1;
    VkDeviceSize texcoord0BufferOffset = -1;
    glTFcomponentType texcoord0ComponentType = static_cast<glTFcomponentType>(-1);
    VkDeviceSize texcoord1BufferOffset = -1;
    glTFcomponentType texcoord1ComponentType = static_cast<glTFcomponentType>(-1);
    VkDeviceSize color0BufferOffset = -1;
    glTFcomponentType color0ComponentType = static_cast<glTFcomponentType>(-1);
    VkDeviceSize joints0BufferOffset = -1;
    glTFcomponentType joints0ComponentType = static_cast<glTFcomponentType>(-1);
    VkDeviceSize weights0BufferOffset = -1;
    glTFcomponentType weights0ComponentType = static_cast<glTFcomponentType>(-1);

    VkPipeline vkGraphicsPipeline;
    Anvil::PipelineLayout* pipelineLayout_ptr;

    uint32_t materialIndex = -1;
    MaterialMapsIndexes materialMaps;
};

struct PrimitivesSetSpecs
{
    std::string primitivesSetName;

    ShadersSpecs shaderSpecs;
    bool useMaterial;
    bool useDepthWrite;
    Anvil::CompareOp depthCompare;
};

struct DescriptorSetsCreateInfosPtrsCollection
{
    const Anvil::DescriptorSetCreateInfo* camera_description_set_create_info_ptr;
    const Anvil::DescriptorSetCreateInfo* skins_description_set_create_info_ptr;
    const Anvil::DescriptorSetCreateInfo* materials_description_set_create_info_ptr;
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

    void FlashDevice();

    void InitPrimitivesSet(PrimitivesSetSpecs in_primitives_set_specs,
                           DescriptorSetsCreateInfosPtrsCollection in_descriptor_sets_create_infos_ptrs_collection,
                           Anvil::RenderPass* renderpass_ptr,
                           Anvil::SubPassID subpassID);

    size_t GetPrimitivesCount();
    bool IsPrimitiveTransparent(size_t primitive_index);

    void StartRecordOBBtree();

    OBBtree GetOBBtreeAndReset();

    const std::vector<PrimitiveSpecificSetInfo>& GetPrimitivesSetInfos(std::string in_primitives_set_name) const;
    const std::vector<PrimitiveGeneralInfo>& GetPrimitivesGeneralInfos() const;

    Anvil::Buffer* GetIndexBufferPtr() { return indexBuffer_uptr.get(); }
    Anvil::Buffer* GetPositionBufferPtr() { return positionBuffer_uptr.get(); }
    Anvil::Buffer* GetNormalBufferPtr() { return normalBuffer_uptr.get(); }
    Anvil::Buffer* GetTangentBufferPtr() { return tangentBuffer_uptr.get(); }
    Anvil::Buffer* GetTexcoord0BufferPtr() { return texcoord0Buffer_uptr.get(); }
    Anvil::Buffer* GetTexcoord1BufferPtr() { return texcoord1Buffer_uptr.get(); }
    Anvil::Buffer* GetColor0BufferPtr() { return color0Buffer_uptr.get(); }
    Anvil::Buffer* GetJoints0BufferPtr() { return joints0Buffer_uptr.get(); }
    Anvil::Buffer* GetWeights0BufferPtr() { return weights0Buffer_uptr.get(); }

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
    Anvil::BufferUniquePtr joints0Buffer_uptr;
    Anvil::BufferUniquePtr weights0Buffer_uptr;

    std::vector<unsigned char> localIndexBuffer;
    std::vector<unsigned char> localPositionBuffer;
    std::vector<unsigned char> localNormalBuffer;
    std::vector<unsigned char> localTangentBuffer;
    std::vector<unsigned char> localTexcoord0Buffer;
    std::vector<unsigned char> localTexcoord1Buffer;
    std::vector<unsigned char> localColor0Buffer;
    std::vector<unsigned char> localJoints0Buffer;
    std::vector<unsigned char> localWeights0Buffer;

    std::vector<PrimitiveCPUdata> recorderPrimitivesCPUdatas;
    bool recordingOBBtree = false;

    std::unordered_map<std::string, std::vector<PrimitiveSpecificSetInfo>> primitivesSetsNameToVector_umap;
    std::vector<PrimitiveGeneralInfo> primitivesGeneralInfos;

    std::vector<bool> primitivesTransparencyFlags;

    PipelinesFactory* pipelinesFactory_ptr;
    ShadersSetsFamiliesCache* shadersSetsFamiliesCache_ptr;
    MaterialsOfPrimitives* materialsOfPrimitives_ptr;

    Anvil::BaseDevice* const device_ptr;
    bool hasBeenFlashed = false;
};

