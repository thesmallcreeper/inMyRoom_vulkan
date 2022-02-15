#pragma once

#include <vector>

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"
#include "tiny_gltf.h"

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
    SkinsOfMeshes(vk::Device device,
                  vma::Allocator vma_allocator);
    ~SkinsOfMeshes();

    void AddSkinsOfModel(const tinygltf::Model& model);
    void FlashDevice(std::pair<vk::Queue, uint32_t> queue );

    size_t GetSkinIndexOffsetOfModel(const tinygltf::Model& in_model) const;

    const SkinInfo& GetSkin(size_t index) const {return skinInfos[index];}

    size_t GetCountOfInverseBindMatrices() const;
    vk::DescriptorSet GetDescriptorSet() const {return descriptorSet;}
    vk::DescriptorSetLayout GetDescriptorSetLayout() const {return descriptorSetLayout;}
private:
    static glm::mat4 GetAccessorMatrix(size_t index,
                                       const tinygltf::Model& in_model,
                                       const tinygltf::Accessor& in_accessor);

    size_t GetInverseBindMatricesBufferSize() const;

private: // data

    std::unordered_map<tinygltf::Model*, size_t> modelToSkinIndexOffset_umap;
    std::vector<SkinInfo> skinInfos;

    std::vector<glm::mat4> inverseBindMatrices;

    vk::Device device;
    vma::Allocator vma_allocator;

    vk::Buffer inverseBindBuffer;
    vma::Allocation inverseBindAllocation;
    size_t inverseBindMatricesCount = -1;
    bool hasBeenFlashed = false;

    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorSetLayout descriptorSetLayout;
};