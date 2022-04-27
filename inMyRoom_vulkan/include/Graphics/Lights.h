#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "hash_combine.h"

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "Geometry/Sphere.h"
#include "Geometry/Paralgram.h"

#include "common/structs/LightParameters.h"

#include "ECS/ECStypes.h"

struct LightsIndicesRange
{
    uint32_t offset = 0;
    uint32_t size = 0;
};

class Lights
{
public:
    Lights(class Graphics* in_graphics_ptr,
           vk::Device device, vma::Allocator vma_allocator,
           size_t max_lights, size_t max_lightCombinationsSize);
    ~Lights();

    size_t AddLightEntity();
    void RemoveLightEntitySafe(size_t index);

    void PrepareNewFrame(size_t frame_index);
    void AddLights(std::vector<LightInfo>& light_infos,
                   const std::vector<ModelMatrices>& model_matrices);
    LightsIndicesRange CreateLightsConesRange();
    LightsIndicesRange CreateCollidedLightsRange(const Paralgram& paralgram);
    void WriteLightsBuffers() const;

    const LightInfo& GetLightInfo(size_t light_index) const;

    // Descriptor binding 0 -> Lights Parameters
    // Descriptor binding 1 -> Light Range
    vk::DescriptorSet GetDescriptorSet() const {return descriptorSets[frameIndex % 3];}
    vk::DescriptorSetLayout GetDescriptorSetLayout() const {return descriptorSetLayout;}

    size_t GetMaxLights() const {return max_lights;}
    size_t GetLightsCombinationsSize() const {return max_lightCombinationsSize;}

    glm::vec3 GetUniformLuminance() const {return uniformLuminance;}

private:
    void InitBuffers();
    void InitDescriptors();

    std::vector<uint16_t> CollideParalgramWithLocalLights(const Paralgram& paralgram);

private:
    std::unordered_set<size_t> lights_indices_uset;
    size_t indexCounter = 0;

    std::unordered_map<size_t, LightInfo> lightIndexToLightInfo_umap;

    std::vector<LightParameters> lightParameters;
    std::vector<Sphere> lightsSpheres;

    std::unordered_map<std::vector<uint16_t>, LightsIndicesRange> combinationToLightsIndicesRange;
    std::vector<uint16_t> lightsCombinationsIndices;

    glm::vec3 uniformLuminance = glm::vec3(0.f);

    vk::Buffer lightsBuffer;
    vma::Allocation lightsAllocation;
    vma::AllocationInfo lightsAllocInfo;
    size_t lightsRangeSize;

    vk::Buffer lightsCombinationsBuffer;
    vma::Allocation lightsCombinationsAllocation;
    vma::AllocationInfo lightsCombinationsAllocInfo;
    size_t lightsCombinationsRangeSize;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       descriptorSets[3];
    vk::DescriptorSetLayout descriptorSetLayout;

    vk::Device device;
    vma::Allocator vma_allocator;

    const size_t max_lights;
    const size_t max_lightCombinationsSize;

    size_t frameIndex = 0;

    class Graphics* const graphics_ptr;
};