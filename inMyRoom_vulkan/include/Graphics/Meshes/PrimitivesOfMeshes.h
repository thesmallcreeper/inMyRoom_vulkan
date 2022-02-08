#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "tiny_gltf.h"

#include "Geometry/OBBtree.h"
#include "Geometry/Triangle.h"

#include "Graphics/Meshes/MaterialsOfPrimitives.h"

// TODO: fallback when no normal or tangent

struct PrimitiveInfo
{
    vk::PrimitiveTopology drawMode = vk::PrimitiveTopology::eTriangleList;

    size_t material                 =  0;
    bool materialTwoSided           =  false;

    size_t indicesCount             =  0;
    size_t verticesCount            =  0;

    VkDeviceSize indicesByteOffset  = -1;

    int positionMorphTargets        =  0;
    VkDeviceSize positionByteOffset = -1;

    int normalMorphTargets          =  0;
    VkDeviceSize normalByteOffset   = -1;

    int tangentMorphTargets         =  0;
    VkDeviceSize tangentByteOffset  = -1;

    int texcoordsCount              =  0;
    int texcoordsMorphTargets       =  0;
    VkDeviceSize texcoordsByteOffset= -1;

    int colorMorphTargets           =  0;
    VkDeviceSize colorByteOffset    = -1;

    int jointsCount                 =  0;
    VkDeviceSize jointsByteOffset   = -1;

    int weightsCount                =  0;
    VkDeviceSize weightsByteOffset  = -1;
};

class PrimitivesOfMeshes
{
    struct PrimitiveOBBtreeData
    {
        bool isSkinOrMorph = false;

        std::vector<glm::vec3> points;
        std::vector<glm::vec3> normals;
        std::vector<uint32_t> indices;
        glTFmode drawMode;
    };

    class PrimitiveInitializationData
    {
    public:
        glTFmode drawMode       = glTFmode::triangles;
        std::vector<uint32_t>   indices;

        int positionMorphTargets= 0;
        std::vector<float>      position;   // glm::vec4

        int normalMorphTargets  = 0;
        std::vector<float>      normal;     // glm::vec4

        int tangentMorphTargets = 0;
        std::vector<float>      tangent;    // glm::vec4

        int texcoordsCount      = 0;
        int texcoordsMorphTargets = 0;
        std::vector<float>      texcoords;  // glm::vec2

        // Maximum 1 color
        int colorMorphTargets   = 0;
        std::vector<float>      color;      // glm::vec4

        int jointsCount         = 0;
        std::vector<uint16_t>   joints;     // uint16_t[4]

        int weightsCount        = 0;
        std::vector<float>      weights;    // glm::vec4

        size_t material         = 0;

    public:
        PrimitiveInitializationData() = default;
        PrimitiveInitializationData(const tinygltf::Model& model,
                                    const tinygltf::Primitive& primitive,
                                    const MaterialsOfPrimitives* materialsOfPrimitives_ptr);

        PrimitiveOBBtreeData GetPrimitiveOBBtreeData() const;

        size_t IndicesBufferSize() const;
        size_t VerticesBufferSize() const;

    private:
        static std::pair<const std::byte*, const std::byte*> GetAccessorBeginEndPtrs(const tinygltf::Model& model,
                                                                                     const tinygltf::Accessor& accessor);
    };
public:
    PrimitivesOfMeshes(MaterialsOfPrimitives* materialsOfPrimitives_ptr,
                       vk::Device device,
                       vma::Allocator allocator);
    ~PrimitivesOfMeshes();

    void AddDefaultPrimitive();

    size_t AddPrimitive(const tinygltf::Model& model,
                        const tinygltf::Primitive& primitive);

    void FlashDevice(std::pair<vk::Queue, uint32_t> queue);

    void StartRecordOBBtree();
    OBBtree GetOBBtreeAndReset();

    size_t GetPrimitivesCount() const {return primitivesInfo.size();}
    const PrimitiveInfo& GetDefaultPrimitiveInfo() const {return primitivesInfo[0];}
    const PrimitiveInfo& GetPrimitiveInfo(size_t index) const {return primitivesInfo[index];}
    vk::Buffer GetBuffer() const {return buffer;}

    bool IsPrimitiveSkinned(size_t index) const;
    size_t PrimitiveMorphTargetsCount(size_t index) const;

    std::tuple<bool, vk::AccelerationStructureGeometryKHR, vk::AccelerationStructureBuildRangeInfoKHR> GetPrimitiveAccelerationStructureTriangle(size_t index);

private:
    size_t GetIndicesBufferSize() const;
    size_t GetVerticesBufferSize() const;

    void InitializePrimitivesInfo();
    void CopyIndicesToBuffer(std::byte* ptr);
    void CopyVerticesToBuffer(std::byte* ptr, size_t offset);
    void FinishInitializePrimitivesInfo();

    static std::vector<uint32_t> TransformIndicesStripToList(const std::vector<uint32_t>& indices);
    static std::vector<uint32_t> TransformIndicesFansToList(const std::vector<uint32_t>& indices);

private: // data
    std::vector<PrimitiveInfo> primitivesInfo;
    std::vector<PrimitiveInitializationData> primitivesInitializationData;

    std::vector<PrimitiveOBBtreeData> recorderPrimitivesOBBtreeDatas;
    bool recordingOBBtree = false;

    vk::Device device;
    vma::Allocator vma_allocator;

    vk::Buffer buffer;
    vma::Allocation allocation;
    vk::Buffer verticesBuffer;
    vma::Allocation verticesAllocation;
    bool hasBeenFlashed = false;

    MaterialsOfPrimitives* materialsOfPrimitives_ptr;
};

