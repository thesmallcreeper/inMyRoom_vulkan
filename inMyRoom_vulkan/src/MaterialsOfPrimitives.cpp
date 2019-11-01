#include "MaterialsOfPrimitives.h"

#include "glm/vec4.hpp"

MaterialsOfPrimitives::MaterialsOfPrimitives(TexturesOfMaterials* in_materialsTextures_ptr,
                                             Anvil::BaseDevice* const in_device_ptr)
    :
    texturesOfMaterials_ptr(in_materialsTextures_ptr),
    device_ptr(in_device_ptr),
    materialsSoFar(0)
{
}

void MaterialsOfPrimitives::AddMaterialsOfModel(const tinygltf::Model& in_model)
{
    if (!in_model.materials.empty())
    {
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dsg_create_infos_ptr;

        for (const tinygltf::Material& this_material : in_model.materials)
        {
            ShadersSpecs this_materialShaderSpecs;
            this_materialShaderSpecs.emptyDefinition.emplace_back("USE_MATERIAL");

            std::vector<std::pair<VkDeviceSize, VkDeviceSize>> this_loop_uniform_bindings_temp;
            std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> this_loop_textures_bindings;

            Anvil::DescriptorSetCreateInfoUniquePtr this_descriptorSetCreateInfo_ptr = Anvil::DescriptorSetCreateInfo::create();

            size_t bindingCount = 0;
            {
                VkDeviceSize uniform_offset_ptr = static_cast<VkDeviceSize>(localMaterialsFactorsBuffer.size());
                VkDeviceSize uniform_size = 0;

                {
                    auto search = this_material.values.find("baseColorFactor");

                    assert(search != this_material.values.end());

                    tinygltf::ColorValue base_color_factors = search->second.ColorFactor();

                    glm::vec4 this_material_base_color_factors(static_cast<float>(base_color_factors[0]), static_cast<float>(base_color_factors[1]), static_cast<float>(base_color_factors[2]), static_cast<float>(base_color_factors[3]));


                    std::copy(reinterpret_cast<unsigned char*>(&this_material_base_color_factors),
                              reinterpret_cast<unsigned char*>(&this_material_base_color_factors) + sizeof(glm::vec4),
                              std::back_inserter(localMaterialsFactorsBuffer));

                    uniform_size += sizeof(glm::vec4);
                }

                this_descriptorSetCreateInfo_ptr->add_binding(bindingCount++,
                                                              Anvil::DescriptorType::UNIFORM_BUFFER,
                                                              1,
                                                              Anvil::ShaderStageFlagBits::FRAGMENT_BIT);

                std::pair<VkDeviceSize, VkDeviceSize> this_uniform_bind_temp(
                    uniform_offset_ptr,
                    uniform_size
                );

                this_loop_uniform_bindings_temp.emplace_back(this_uniform_bind_temp);
            }
            {
                auto search = this_material.values.find("baseColorTexture");

                if (search != this_material.values.end())
                {
                    this_materialShaderSpecs.emptyDefinition.emplace_back("USE_BASE_COLOR_TEXTURE_TEXCOORD0");

                    size_t this_baseColorTextureIndex = texturesOfMaterials_ptr->GetTextureIndexOffsetOfModel(in_model) + search->second.TextureIndex();

                    this_descriptorSetCreateInfo_ptr->add_binding(bindingCount++,
                                                                  Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                                  1,
                                                                  Anvil::ShaderStageFlagBits::FRAGMENT_BIT);

                    Anvil::DescriptorSet::CombinedImageSamplerBindingElement this_texture_bind(
                        Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                        texturesOfMaterials_ptr->GetImageView(this_baseColorTextureIndex),
                        texturesOfMaterials_ptr->GetSampler(this_baseColorTextureIndex)
                    );

                    this_loop_textures_bindings.emplace_back(this_texture_bind);
                }
            }
            materialsShadersSpecs.emplace_back(this_materialShaderSpecs);

            uniform_bindings_offset_and_size.emplace_back(std::move(this_loop_uniform_bindings_temp));
            textures_bindings.emplace_back(std::move(this_loop_textures_bindings));

            dsg_create_infos_ptr.emplace_back(std::move(this_descriptorSetCreateInfo_ptr));
        }

        materialsDescriptorSetGroup_uptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                                             dsg_create_infos_ptr,
                                                                             false); /* in_releaseable_sets */
    }

    modelToMaterialIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), materialsSoFar);

    materialsSoFar += in_model.materials.size();
}

void MaterialsOfPrimitives::FlashDevice()
{
    // Create and flash the f buffer
    materialsFactorsBuffer_uptr = CreateDeviceBufferForLocalBuffer(std::ref(localMaterialsFactorsBuffer), Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT, "Material factors");

    // Prepare uniform_bindings
    for (auto& this_temp_vector : uniform_bindings_offset_and_size)
    {
        std::vector<Anvil::DescriptorSet::UniformBufferBindingElement> this_loop_uniform_bindings;
        for (auto& this_temp : this_temp_vector)
        {
            Anvil::DescriptorSet::UniformBufferBindingElement this_uniform_bind(
                materialsFactorsBuffer_uptr.get(),
                this_temp.first,
                this_temp.second
            );
            this_loop_uniform_bindings.emplace_back(this_uniform_bind);
        }
        uniform_bindings.emplace_back(std::move(this_loop_uniform_bindings));
    }

    for (uint32_t set_index = 0; set_index < uniform_bindings.size(); set_index++)
    {
        for (uint32_t binding_index = 0; binding_index < uniform_bindings[set_index].size(); binding_index++)
        {
            materialsDescriptorSetGroup_uptr->set_binding_item(set_index,
                                                               binding_index,
                                                               uniform_bindings[set_index][binding_index]);
        }
    }

    for (uint32_t set_index = 0; set_index < textures_bindings.size(); set_index++)
    {
        for (uint32_t binding_index = 0; binding_index < textures_bindings[set_index].size(); binding_index++)
        {
            materialsDescriptorSetGroup_uptr->set_binding_item(set_index,
                                                               binding_index + uniform_bindings[set_index].size(),
                                                               textures_bindings[set_index][binding_index]);
        }
    }
}

size_t MaterialsOfPrimitives::GetMaterialIndexOffsetOfModel(const tinygltf::Model& in_model)
{
    auto search = modelToMaterialIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToMaterialIndexOffset_umap.end());

    return search->second;
}

ShadersSpecs MaterialsOfPrimitives::GetShaderSpecsNeededForMaterial(size_t in_material_index)
{
    return materialsShadersSpecs[in_material_index];
}

const Anvil::DescriptorSetCreateInfo* MaterialsOfPrimitives::GetDescriptorSetCreateInfoPtr(size_t in_material_index)
{
    return materialsDescriptorSetGroup_uptr->get_descriptor_set_create_info(static_cast<uint32_t>(in_material_index));
}

Anvil::DescriptorSet* MaterialsOfPrimitives::GetDescriptorSetPtr(size_t in_material_index)
{
    return materialsDescriptorSetGroup_uptr->get_descriptor_set(static_cast<uint32_t>(in_material_index));
}

Anvil::BufferUniquePtr MaterialsOfPrimitives::CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer, Anvil::BufferUsageFlagBits in_bufferusageflag, std::string buffers_name) const
{
    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    in_localBuffer.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    in_bufferusageflag);

    Anvil::BufferUniquePtr buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

    buffer_ptr->set_name(buffers_name);

    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    allocator_ptr->add_buffer(buffer_ptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE);

    buffer_ptr->write(0,
                      in_localBuffer.size(),
                      in_localBuffer.data());

    return std::move(buffer_ptr);
}
