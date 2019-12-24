#pragma once
#include <vector>
#include <unordered_map>

#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/command_buffer.h"

#include "glTFenum.h"

#include "glm/mat4x4.hpp"

#include "Meshes/PrimitivesOfMeshes.h"

struct DrawRequest
{
    bool isSkin;
    uint32_t objectID;
    size_t primitiveIndex;
    union
    {
        glm::mat4x4 TRSmatrix;
        struct
        {
            uint32_t inverseBindMatricesOffset;
            uint32_t nodesMatricesOffset;
        };
    } vertexData;
};

struct DescriptorSetsPtrsCollection
{
    Anvil::DescriptorSet* camera_description_set_ptr;
    Anvil::DescriptorSet* skin_description_set_ptr;
    Anvil::DescriptorSet* materials_description_set_ptr;
};

struct CommandBufferState
{
    uint32_t objectID = -1;
    std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs;
    VkDeviceSize indexBufferOffset = -1;
    VkDeviceSize positionBufferOffset = -1;
    VkDeviceSize normalBufferOffset = -1;
    VkDeviceSize tangentBufferOffset = -1;
    VkDeviceSize texcoord0BufferOffset = -1;
    VkDeviceSize texcoord1BufferOffset = -1;
    VkDeviceSize color0BufferOffset = -1;
    VkDeviceSize joints0BufferOffset = -1;
    VkDeviceSize weights0BufferOffset = -1;
    Anvil::IndexType indexBufferType = static_cast<Anvil::IndexType>(-1);
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(nullptr);
};

class Drawer
{
public: //functions
    Drawer(std::string primitives_set_sorted_by_name,
           PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
           Anvil::BaseDevice* const device_ptr);

    void AddDrawRequests(std::vector<DrawRequest> in_draw_requests);

    void DrawCallRequests(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                          std::string in_primitives_set_mame,
                          const DescriptorSetsPtrsCollection in_descriptor_sets_ptrs_collection);

private: //functions
    void DrawCall(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                  const DescriptorSetsPtrsCollection& in_descriptor_sets_ptrs_collection,
                  const std::vector<PrimitiveSpecificSetInfo>& in_primitives_set_infos,
                  const DrawRequest in_draw_request,
                  CommandBufferState& ref_command_buffer_state);

    static size_t GetSizeOfComponent(glTFcomponentType component_type);

private: // variebles
    const std::string primitivesSetSortedByName;

    std::unordered_map<VkPipeline, std::vector<DrawRequest >> by_pipeline_VkPipelineToDrawRequests_umap;

    PrimitivesOfMeshes* primitivesOfMeshes_ptr;

    Anvil::BaseDevice* const device_ptr;
};