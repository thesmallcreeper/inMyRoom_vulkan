#include "Graphics/Meshes/MaterialsOfPrimitives.h"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/descriptor_set_create_info.h"

#include "glm/vec4.hpp"

MaterialsOfPrimitives::MaterialsOfPrimitives(TexturesOfMaterials* in_materialsTextures_ptr,
                                             Anvil::BaseDevice* const in_device_ptr)
    :
    texturesOfMaterials_ptr(in_materialsTextures_ptr),
    device_ptr(in_device_ptr)
{
}

void MaterialsOfPrimitives::AddMaterialsOfModel(const tinygltf::Model& in_model)
{
    assert(!hasBeenFlased);

    for (const tinygltf::Material& this_material : in_model.materials)
    {
        MaterialAbout this_materialAbout;

        ShadersSpecs this_materialShaderSpecs;
        this_materialShaderSpecs.emptyDefinition.emplace_back("USE_MATERIAL");       

        {
            // Useful only at MASK but is needed to avoid preprocessor errors
            this_materialShaderSpecs.definitionStringPairs.emplace_back(std::make_pair("ALPHA_CUTOFF", std::to_string(this_material.alphaCutoff)));

            if (this_material.alphaMode == "BLEND")
            {
                this_materialAbout.transparent = true;
                this_materialShaderSpecs.emptyDefinition.emplace_back("IS_TRANSPARENT");
            }
            else if (this_material.alphaMode == "MASK")
            {
                this_materialAbout.transparent = false;
                this_materialShaderSpecs.emptyDefinition.emplace_back("IS_MASKED");
            }
            else if(this_material.alphaMode == "OPAQUE")
            {
                this_materialAbout.transparent = false;
                this_materialShaderSpecs.emptyDefinition.emplace_back("IS_OPAQUE");
            }

            if (this_material.doubleSided)
                this_materialAbout.twoSided = true;
            else
                this_materialAbout.twoSided = false;
        }

        {
            MaterialParameters this_materialParameters;

            {
                auto search = this_material.values.find("baseColorFactor");

                if (search != this_material.values.end())
                {
                    tinygltf::ColorValue base_color_factors = search->second.ColorFactor();
                    glm::vec4 this_material_base_color_factors(static_cast<float>(base_color_factors[0]),
                                                               static_cast<float>(base_color_factors[1]),
                                                               static_cast<float>(base_color_factors[2]),
                                                               static_cast<float>(base_color_factors[3]));

                    this_materialParameters.baseColorFactors = this_material_base_color_factors;
                }
                else
                {
                    glm::vec4 this_material_base_color_factors(1.f, 1.f, 1.f, 1.f);

                    this_materialParameters.baseColorFactors = this_material_base_color_factors;
                }
            }

            std::copy(reinterpret_cast<unsigned char*>(&this_materialParameters),
                      reinterpret_cast<unsigned char*>(&this_materialParameters) + sizeof(MaterialParameters),
                      std::back_inserter(localMaterialsParametersBuffer));

        }

        {
            MaterialMapsIndexes this_materialMapsIndexes;

            {
                auto search = this_material.values.find("baseColorTexture");

                if (search != this_material.values.end())
                {
                    this_materialShaderSpecs.emptyDefinition.emplace_back("USE_BASE_COLOR_TEXTURE_TEXCOORD0");

                    uint32_t this_baseColorTextureIndex = static_cast<uint32_t>(search->second.TextureIndex() + texturesOfMaterials_ptr->GetTextureIndexOffsetOfModel(in_model));
                   
                    Anvil::DescriptorSet::CombinedImageSamplerBindingElement this_texture_bind(Anvil::ImageLayout::SHADER_READ_ONLY_OPTIMAL,
                                                                                               texturesOfMaterials_ptr->GetImageView(this_baseColorTextureIndex),
                                                                                               texturesOfMaterials_ptr->GetSampler(this_baseColorTextureIndex));

                    this_materialMapsIndexes.baseColor = static_cast<uint32_t>(texturesBindings.size());
                    texturesBindings.emplace_back(this_texture_bind);
                }   
                materialsMapsIndexes.emplace_back(this_materialMapsIndexes);
            }          
        }

        materialsAbout.emplace_back(this_materialAbout);
        materialsShadersSpecs.emplace_back(this_materialShaderSpecs);
    }

    modelToMaterialIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), materialsSoFar);

    materialsSoFar += in_model.materials.size();
}

void MaterialsOfPrimitives::FlashDevice()
{
    assert(!hasBeenFlased);

    // Create and flash the f buffer
    materialsParametersBuffer_uptr = CreateDeviceBufferForLocalBuffer(localMaterialsParametersBuffer, Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT, "Material Parameters");

    // DS creation
    {
        // Create description set

        Anvil::DescriptorSetCreateInfoUniquePtr this_description_set_create_info_uptr = Anvil::DescriptorSetCreateInfo::create();

        // Add uniform buffer binding (bind: 0)

        this_description_set_create_info_uptr->add_binding(0 /*in_binding_index*/,
                                                           Anvil::DescriptorType::UNIFORM_BUFFER,
                                                           1 /*in_descriptor_array_size*/,
                                                           Anvil::ShaderStageFlagBits::FRAGMENT_BIT);

        // Add textures binding (bind: 1)

        this_description_set_create_info_uptr->add_binding(1, /*in_binding_index*/
                                                           Anvil::DescriptorType::COMBINED_IMAGE_SAMPLER,
                                                           static_cast<uint32_t>(texturesBindings.size()), /*in_descriptor_array_size*/
                                                           Anvil::ShaderStageFlagBits::FRAGMENT_BIT);

        // Create DescriptorSetGroup
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dsg_create_infos_uptrs;
        dsg_create_infos_uptrs.emplace_back(std::move(this_description_set_create_info_uptr));

        descriptorSetGroup_uptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                                    dsg_create_infos_uptrs,
                                                                    false);
    }

    // Bind materials parameters (to bind: 0)
    {
        Anvil::DescriptorSet::UniformBufferBindingElement this_uniform_bind(materialsParametersBuffer_uptr.get(),
                                                                            0, /*in_start_offset*/
                                                                            materialsParametersBuffer_uptr->get_create_info_ptr()->get_size());

        descriptorSetGroup_uptr->set_binding_item(0, /*in_n_set*/
                                                  0, /*in_binding_index*/
                                                  this_uniform_bind);
    }

    // Bind sampler+textures (to bind: 1)
    {
        Anvil::BindingElementArrayRange array_range;
        array_range.first = 0;
        array_range.second = static_cast<uint32_t>(texturesBindings.size());

        descriptorSetGroup_uptr->set_binding_array_items(0, /*in_n_set*/
                                                         1, /*in_binding_index*/
                                                         array_range,
                                                         texturesBindings.data());
    }

    // Complete shaders sets creation
    InformShadersSpecsAboutRanges(texturesBindings.size(), materialsSoFar);

    hasBeenFlased = true;
}

MaterialMapsIndexes MaterialsOfPrimitives::GetMaterialMapsIndexes(size_t in_material_index)
{
    return materialsMapsIndexes[in_material_index];
}

ShadersSpecs MaterialsOfPrimitives::GetShaderSpecsNeededForMaterial(size_t in_material_index)
{
    return materialsShadersSpecs[in_material_index];
}

MaterialAbout MaterialsOfPrimitives::GetMaterialAbout(size_t in_material_index)
{
    return materialsAbout[in_material_index];
}

size_t MaterialsOfPrimitives::GetMaterialIndexOffsetOfModel(const tinygltf::Model& in_model)
{
    auto search = modelToMaterialIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToMaterialIndexOffset_umap.end());

    return search->second;
}

const Anvil::DescriptorSetCreateInfo* MaterialsOfPrimitives::GetDescriptorSetCreateInfoPtr()
{
    assert(hasBeenFlased);

    return descriptorSetGroup_uptr->get_descriptor_set_create_info(0);
}

Anvil::DescriptorSet* MaterialsOfPrimitives::GetDescriptorSetPtr()
{
    assert(hasBeenFlased);

    return descriptorSetGroup_uptr->get_descriptor_set(0);
}

Anvil::BufferUniquePtr MaterialsOfPrimitives::CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer, Anvil::BufferUsageFlagBits in_bufferusageflag, std::string buffers_name) const
{
    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    in_localBuffer.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    in_bufferusageflag);

    Anvil::BufferUniquePtr buffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

    buffer_uptr->set_name(buffers_name);

    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    allocator_ptr->add_buffer(buffer_uptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE);

    buffer_uptr->write(0,
                      in_localBuffer.size(),
                      in_localBuffer.data());

    return std::move(buffer_uptr);
}

void MaterialsOfPrimitives::InformShadersSpecsAboutRanges(size_t textures_count, size_t materials_count)
{
    for (ShadersSpecs& this_shaders_specs : materialsShadersSpecs)
    {
        this_shaders_specs.definitionValuePairs.emplace_back(std::make_pair("MATERIALS_PARAMETERS_COUNT", static_cast<int32_t>(materials_count)));
        this_shaders_specs.definitionValuePairs.emplace_back(std::make_pair("TEXTURES_COUNT", static_cast<int32_t>(textures_count)));
    }
}
