#pragma once
#include <vector>
#include <unordered_map>

#include "wrappers/device.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/command_buffer.h"
#include "glTFenum.h"

#include "glm/mat4x4.hpp"

#include "PrimitivesOfMeshes.h"

struct DrawRequest
{
    float z = 0.0f;
    uint32_t objectID;
    size_t primitiveIndex;
    glm::mat4x4 TRSmatrix;
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
    Anvil::IndexType indexBufferType = static_cast<Anvil::IndexType>(-1);
    VkPipeline vkGraphicsPipeline = static_cast<VkPipeline>(nullptr);
};

enum class sorting
{
    none,
    by_increasing_z_depth,
    by_pipeline
};

class Drawer
{
public: //functions
    Drawer(sorting in_sorting_method,
           std::string only_by_pipeline_primitives_set_name,
           PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
           Anvil::BaseDevice* const device_ptr);

    void AddDrawRequests(std::vector<DrawRequest> in_draw_requests);

    void DrawCallRequests(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                          std::string in_primitives_set_mame,
                          const std::vector<Anvil::DescriptorSet*> in_low_descriptor_sets_ptrs);

private: //functions
    void DrawCall(Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr,
                  const std::vector<Anvil::DescriptorSet*>& in_low_descriptor_sets_ptrs,
                  const std::vector<PrimitiveSpecificSetInfo>& in_primitives_set_infos,
                  const std::vector<PrimitiveGeneralInfo>& in_primitives_general_infos,
                  const DrawRequest in_draw_request,
                  CommandBufferState& ref_command_buffer_state);

    static size_t GetSizeOfComponent(glTFcomponentType component_type);

private: // variebles
    const sorting sortingMethod;
    const std::string primitivesSetNameForByPipeline;

    std::vector<DrawRequest> none_drawRequests;
    std::unordered_map<VkPipeline, std::vector<DrawRequest >> by_pipeline_VkPipelineToDrawRequests_umap;

    Anvil::BaseDevice* const device_ptr;
    PrimitivesOfMeshes* primitivesOfMeshes_ptr;
};