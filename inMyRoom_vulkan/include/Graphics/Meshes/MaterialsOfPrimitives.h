#pragma once

#include <string>

#include "vulkan/vulkan.hpp"
#include "tiny_gltf.h"

#include "glm/vec4.hpp"

#include "Graphics/ShadersSetsFamiliesCache.h"
#include "Graphics/Meshes/TexturesOfMaterials.h"
#include "common/structs/MaterialParameters.h"

struct MaterialAbout
{
    bool twoSided = false;
    bool transparent = false;
    bool masked = false;
    uint32_t color_texcooord = 0;
    std::vector<std::pair<std::string, std::string>> definitionStringPairs;
};

class MaterialsOfPrimitives
{
public: // functions
    MaterialsOfPrimitives(TexturesOfMaterials* texturesOfMaterials_ptr,
                          vk::Device device,
                          vma::Allocator allocator);

    ~MaterialsOfPrimitives();

    void AddDefaultTextures();
    void AddDefaultMaterial();

    void AddMaterialsOfModel(const tinygltf::Model& model, const std::string& model_folder);
    size_t GetMaterialIndexOffsetOfModel(const tinygltf::Model& in_model) const;

    size_t GetMaterialsCount() const {return materialsAbout.size();}
    const MaterialAbout& GetMaterialAbout(size_t index) const {return materialsAbout[index];}

    void FlashDevice(std::pair<vk::Queue, uint32_t> queue);

    vk::DescriptorSet GetDescriptorSet() const {return descriptorSet;}
    vk::DescriptorSetLayout GetDescriptorSetLayout() const {return descriptorSetLayout;}

private: // functions
    void InformShadersSpecsAboutRanges(size_t textures_count, size_t materials_count);
    size_t GetMaterialParametersBufferSize() const;

private: // data
    std::vector<MaterialParameters> materialsParameters;
    std::vector<MaterialAbout> materialsAbout;

    // Default textures
    uint32_t defaultColorTextureIndex = -1;

    // set: 0, bind: 0, UBO with material parameters
    // set: 0, bind: 1, array Sampler+Image
    vk::DescriptorPool descriptorPool;
    vk::DescriptorSet descriptorSet;
    vk::DescriptorSetLayout descriptorSetLayout;

    std::unordered_map<tinygltf::Model*, size_t> modelToMaterialIndexOffset_umap;

    TexturesOfMaterials* texturesOfMaterials_ptr;

    vk::Device device;
    vma::Allocator vma_allocator;

    vk::Buffer materialParametersBuffer;
    vma::Allocation materialParametersAllocation;
    bool hasBeenFlashed = false;

};
