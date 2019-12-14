#pragma once

#include <string>

#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/buffer.h"

#include "tiny_gltf.h"

#include "glm/vec4.hpp"

#include "ShadersSetsFamiliesCache.h"

#include "Meshes/TexturesOfMaterials.h"

struct MaterialParameters
{
    glm::vec4 baseColorFactors   = glm::vec4(1.f, 1.f, 1.f, 1.f);
    glm::vec4 _placeholder       = glm::vec4(1.f, 1.f, 1.f, 1.f);
};

struct MaterialMapsIndexes
{
    uint32_t baseColor = -1;
    uint32_t metallic = -1;
    uint32_t roughness = -1;
    uint32_t normal = -1;
    uint32_t occlusion = -1;
    uint32_t emissive = -1;
};

class MaterialsOfPrimitives
{
public: // functions
    MaterialsOfPrimitives(TexturesOfMaterials* in_texturesOfMaterials_ptr,
                          Anvil::BaseDevice* const in_device_ptr);

    void AddMaterialsOfModel(const tinygltf::Model& in_model);
    size_t GetMaterialIndexOffsetOfModel(const tinygltf::Model& in_model);

    void FlashDevice();

    MaterialMapsIndexes GetMaterialMapsIndexes(size_t in_material_index);

    ShadersSpecs GetShaderSpecsNeededForMaterial(size_t in_material_index);

    const Anvil::DescriptorSetCreateInfo* GetDescriptorSetCreateInfoPtr();
    Anvil::DescriptorSet* GetDescriptorSetPtr();

private: // functions
    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                            Anvil::BufferUsageFlagBits in_bufferusageflag,
                                                            std::string buffers_name) const;

    void InformShadersSpecsAboutRanges(size_t textures_count, size_t materials_count);

private: // data
    size_t materialsSoFar = 0;

    std::vector<MaterialMapsIndexes> materialsMapsIndexes;

    std::vector<ShadersSpecs> materialsShadersSpecs;

    // set: 0, bind: 0, UBO with material parameters
    // set: 0, bind: 1, array Sampler+Image
    Anvil::DescriptorSetGroupUniquePtr descriptorSetGroup_uptr;

    Anvil::BufferUniquePtr materialsParametersBuffer_uptr;
    std::vector<unsigned char> localMaterialsParametersBuffer;

    std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> texturesBindings;

    std::unordered_map<tinygltf::Model*, size_t> modelToMaterialIndexOffset_umap;

    TexturesOfMaterials* texturesOfMaterials_ptr;

    Anvil::BaseDevice* const device_ptr;
    bool hasBeenFlased = false;
};
