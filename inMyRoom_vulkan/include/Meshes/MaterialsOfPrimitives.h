#pragma once

#include <string>

#include "wrappers/descriptor_set.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/buffer.h"

#include "tiny_gltf.h"

#include "ShadersSetsFamiliesCache.h"

#include "Meshes/TexturesOfMaterials.h"

class MaterialsOfPrimitives
{
public: // functions
    MaterialsOfPrimitives(TexturesOfMaterials* in_texturesOfMaterials_ptr,
                          Anvil::BaseDevice* const in_device_ptr);

    void AddMaterialsOfModel(const tinygltf::Model& in_model);
    size_t GetMaterialIndexOffsetOfModel(const tinygltf::Model& in_model);

    void FlashDevice();

    ShadersSpecs GetShaderSpecsNeededForMaterial(size_t in_material_index);
    const Anvil::DescriptorSetCreateInfo* GetDescriptorSetCreateInfoPtr(size_t in_material_index);
    Anvil::DescriptorSet* GetDescriptorSetPtr(size_t in_material_index);

private: // functions
    Anvil::BufferUniquePtr CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                            Anvil::BufferUsageFlagBits in_bufferusageflag,
                                                            std::string buffers_name) const;
private: // data
    size_t materialsSoFar;

    Anvil::DescriptorSetGroupUniquePtr materialsDescriptorSetGroup_uptr;

    std::vector<ShadersSpecs> materialsShadersSpecs;

    Anvil::BufferUniquePtr materialsFactorsBuffer_uptr;
    std::vector<unsigned char> localMaterialsFactorsBuffer;

    std::unordered_map<tinygltf::Model*, size_t> modelToMaterialIndexOffset_umap;

    std::vector<std::vector<std::pair<VkDeviceSize, VkDeviceSize>>> uniform_bindings_offset_and_size;  // We need temps because we cannot create uniform buffer beforehand
    std::vector<std::vector<Anvil::DescriptorSet::UniformBufferBindingElement>> uniform_bindings;

    std::vector<std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement>> textures_bindings;

    TexturesOfMaterials* texturesOfMaterials_ptr;

    Anvil::BaseDevice* const device_ptr;
};
