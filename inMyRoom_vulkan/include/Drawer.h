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
    Anvil::PipelineID pipeline_id = -1;
    std::vector<Anvil::DescriptorSet*> descriptor_sets_ptrs;
    VkDeviceSize indexBufferOffset = -1;
    VkDeviceSize positionBufferOffset = -1;
    VkDeviceSize normalBufferOffset = -1;
    VkDeviceSize tangentBufferOffset = -1;
    VkDeviceSize texcoord0BufferOffset = -1;
    VkDeviceSize texcoord1BufferOffset = -1;
    VkDeviceSize color0BufferOffset = -1;
    Anvil::IndexType indexBufferType = static_cast<Anvil::IndexType>(-1);
};

enum class sorting
{
    none,
    by_increasing_z_depth,
    by_pipeline
};

class Drawer
{
public:
    Drawer(sorting in_sorting_method, size_t in_pipelineSetIndex, PrimitivesOfMeshes* in_primitivesOfMeshes_ptr, Anvil::BaseDevice* const device_ptr);   // gotta do it primitivesOfmeshes independent
    ~Drawer();

    void AddDrawRequests(std::vector<DrawRequest> in_draw_requests);

    void DrawCallRequests(std::vector<std::pair<Anvil::PrimaryCommandBuffer*, size_t>> pairs_primaryCommandBuffers_primitiveSets,
                          const std::vector<Anvil::DescriptorSet*>& in_low_descriptor_sets_ptrs) const;
    void DeleteDrawRequests();

private:
    void DrawCall(CommandBufferState& command_buffer_state, Anvil::PrimaryCommandBuffer* in_cmd_buffer_ptr, size_t in_primitivesSet_index,
                  const DrawRequest draw_request, const std::vector<Anvil::DescriptorSet*>& in_low_descriptor_sets_ptrs) const;
    static size_t GetSizeOfComponent(glTFcomponentType component_type);

    const sorting sortingMethod;
    const size_t pipelineSetIndex;

    std::vector<DrawRequest> none_drawRequests;
    std::unordered_map<Anvil::PipelineID, std::vector<DrawRequest >> by_pipeline_PipelineIDToDrawRequests_umap;

    Anvil::BaseDevice* const device_ptr;
    PrimitivesOfMeshes* primitivesOfMeshes_ptr;
};