#include "PrimitivesMaterials.h"

PrimitivesMaterials::PrimitivesMaterials(tinygltf::Model& in_model, MaterialsTextures* in_materialsTextures_ptr, Anvil::BaseDevice* in_device_ptr)
    :
    materialsTextures_ptr(in_materialsTextures_ptr),
    device_ptr(in_device_ptr)
{
    if (!in_model.materials.empty())
    {
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dsg_create_infos_ptr;

        std::vector<std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement>> textures_bindings;

        for (tinygltf::Material& this_material : in_model.materials)
        {
            ShadersSpecs this_materialShaderSpecs;

            std::vector<Anvil::DescriptorSet::CombinedImageSamplerBindingElement> this_loop_textures_bindings;

            Anvil::DescriptorSetCreateInfoUniquePtr this_descriptorSetCreateInfo_ptr = Anvil::DescriptorSetCreateInfo::create();
            size_t bindingCount = 0;

            {
                auto search = this_material.values.find("baseColorTexture");

                if (search != this_material.values.end())
                {
                    this_materialShaderSpecs.emptyDefinition.emplace_back("FRAG_BASE_COLOR_TEXTURE_TEXCOORD0");

                    auto this_baseColorTextureIndex = search->second.TextureIndex();

                    TextureInfo this_texture_info = materialsTextures_ptr->textures[this_baseColorTextureIndex];

                    this_descriptorSetCreateInfo_ptr->add_binding(bindingCount++,
                                                                  Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                                  1,
                                                                  Anvil::ShaderStageFlagBits::FRAGMENT_BIT);

                    Anvil::DescriptorSet::CombinedImageSamplerBindingElement this_texture_bind(Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                                                               materialsTextures_ptr->imagesViews[this_texture_info.imageIndex].get(),
                                                                                               materialsTextures_ptr->samplers[this_texture_info.samplerIndex].get());

                    this_loop_textures_bindings.emplace_back(this_texture_bind);
                }

            }
            materialsShadersSpecs.emplace_back(this_materialShaderSpecs);

            textures_bindings.emplace_back(this_loop_textures_bindings);
            dsg_create_infos_ptr.emplace_back(std::move(this_descriptorSetCreateInfo_ptr));

        }

        dsg_ptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                    dsg_create_infos_ptr,
                                                    false); /* in_releaseable_sets */

        for (size_t set_index = 0; set_index < textures_bindings.size(); set_index++)
        {
            for (size_t binding_index = 0; binding_index < textures_bindings[set_index].size(); binding_index++)
            {
                dsg_ptr->set_binding_item(set_index,
                                          binding_index,
                                          textures_bindings[set_index][binding_index]);

            }
        }
    }
}

PrimitivesMaterials::~PrimitivesMaterials()
{
    dsg_ptr.reset();
    materialsFactorsBuffer_ptrs.clear();
    materialsShadersSpecs.clear();
}