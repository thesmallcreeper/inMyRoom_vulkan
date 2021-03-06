#pragma once

#include <vector>

#include "tiny_gltf.h"

#include "wrappers/device.h"
#include "wrappers/buffer.h"
#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"

#include "glm/mat4x4.hpp"

struct SkinInfo
{
    size_t inverseBindMatricesFirstOffset;
    size_t inverseBindMatricesSize;
    std::vector<size_t> glTFnodesJoints;
};

class SkinsOfMeshes
{
public:
    SkinsOfMeshes(Anvil::BaseDevice* const in_device_ptr,
                  size_t swapchain_size,
                  size_t in_nodes_buffer_size);
    ~SkinsOfMeshes();

    void AddSkinsOfModel(const tinygltf::Model& in_model);
    void FlashDevice();

    size_t GetSkinIndexOffsetOfModel(const tinygltf::Model& in_model) const;

    SkinInfo GetSkin(size_t this_skin_index) const;

    void StartRecordingNodesMatrices();

    void AddNodeMatrix(glm::mat4 in_node_matrix);
    size_t GetNodesRecordSize() const;

    void EndRecodingAndFlash(size_t in_swapchain, Anvil::Queue* in_opt_flash_queue = nullptr);

    const Anvil::DescriptorSetCreateInfo* GetDescriptorSetCreateInfoPtr();
    Anvil::DescriptorSet* GetDescriptorSetPtr(size_t in_swapchain);

    size_t GetMaxCountOfNodesMatrices() const;
    size_t GetMaxCountOfInverseBindMatrices() const;
private:
    glm::mat4 GetAccessorMatrix(const size_t index,
                                const tinygltf::Model& in_model,
                                const tinygltf::Accessor& in_accessor);

    void AddMatrixToInverseBindMatricesBuffer(const glm::mat4 this_matrix);

    size_t GetCountOfInverseBindMatrices() const;

    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                            Anvil::BufferUsageFlagBits in_bufferusageflag,
                                                            std::string buffers_name) const;

private: // data
    size_t skinsSoFar = 0;

    std::unordered_map<tinygltf::Model*, size_t> modelToSkinIndexOffset_umap;

    std::vector<SkinInfo> skins;

    const size_t swapchainSize;
    const size_t nodesMatricesBufferSize;

    bool isRecording = false;

    std::vector<unsigned char> localCurrectSwapchainNodesMatricesBuffer;
    std::vector<Anvil::BufferUniquePtr> nodesMatricesBuffers_uptrs;

    std::vector<unsigned char> localInverseBindMatricesBuffer;
    Anvil::BufferUniquePtr inverseBindMatricesBuffer_uptr;

    Anvil::DescriptorSetGroupUniquePtr descriptorSetGroup_uptr;

    Anvil::BaseDevice* const device_ptr;
    bool hasInitFlashed = false;
};