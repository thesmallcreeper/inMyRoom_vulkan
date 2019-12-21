#pragma once

#include <vector>

#include "tiny_gltf.h"

#include "wrappers/device.h"
#include "wrappers/buffer.h"

#include "glm/mat4x4.hpp"

struct SkinInfo
{
    size_t inverseBindMatrixesFirstOffset;
    size_t inverseBindMatrixesSize;
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

    void StartRecordingNodesMatrixes(size_t in_swapchain);

    void AddNodeMatrix(glm::mat4 in_node_matrix);
    size_t GetRecordSize() const;

    void EndRecodingAndFlash();

private:
    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                            Anvil::BufferUsageFlagBits in_bufferusageflag,
                                                            std::string buffers_name) const;

private: // data
    std::unordered_map<tinygltf::Model*, size_t> modelToMeshIndexOffset_umap;

    std::vector<SkinInfo> skins;

    const size_t swapchainSize;
    const size_t nodesMatrixesBufferSize;

    bool isRecording = false;
    size_t currectSwapchainBeingRecorded = -1;

    std::vector<unsigned char> localCurrectSwapchainNodesMatrixesBuffer;
    std::vector<Anvil::BufferUniquePtr> nodesMatrixesBuffers_uptrs;

    std::vector<unsigned char> localInverseBindMatrixesBuffer;
    Anvil::BufferUniquePtr inverseBindMatrixesBuffer_uptr;

    Anvil::BaseDevice* const device_ptr;
    bool hasInverseBindMatrixesBeenFlashed = false;
};