#include "Graphics/Meshes/MaterialsOfPrimitives.h"

#include "glm/vec4.hpp"
#include "Graphics/HelperUtils.h"

MaterialsOfPrimitives::MaterialsOfPrimitives(TexturesOfMaterials *in_texturesOfMaterials_ptr,
                                             vk::Device in_device,
                                             vma::Allocator in_allocator)
    :texturesOfMaterials_ptr(in_texturesOfMaterials_ptr),
     device(in_device),
     vma_allocator(in_allocator)
{
    AddDefaultTextures();
    AddDefaultMaterial();
}

void MaterialsOfPrimitives::AddDefaultTextures()
{
    // Color texture
    {
        ImageData image(4, 4, 4, glTFsamplerWrap::repeat, glTFsamplerWrap::repeat);

        std::vector<uint8_t> image_data(4 * 4 * 4, 0);
        image.SetImage(image_data, false);

        defaultColorTextureIndex = texturesOfMaterials_ptr->AddTextureAndMipmaps(std::vector(1, image),
                                                                                 vk::Format::eR8G8B8A8Srgb);

    }
    // Normal texture
    {
        ImageData image(4, 4, 4, glTFsamplerWrap::repeat, glTFsamplerWrap::repeat);

        std::vector<uint16_t> image_data(4 * 4 * 4, 0);
        for(size_t i = 0; i < image_data.size(); i += 4) {
            image_data[i]     = uint16_t(-1) >> 1;
            image_data[i + 1] = uint16_t(-1) >> 1;
            image_data[i + 2] = uint16_t(-1);
            image_data[i + 3] = uint16_t(-1);
        }
        image.SetImage(image_data);

        defaultNormalTextureIndex = texturesOfMaterials_ptr->AddTextureAndMipmaps(std::vector(1, image),
                                                                                  vk::Format::eA2R10G10B10UnormPack32);
    }
}

void MaterialsOfPrimitives::AddDefaultMaterial()
{
    // Default material
    MaterialAbout this_materialAbout;
    MaterialParameters this_materialParameters;

    this_materialAbout.twoSided = false;
    this_materialAbout.transparent = false;
    this_materialAbout.definitionStringPairs.emplace_back("IS_OPAQUE", "");

    this_materialParameters.baseColorTexture = defaultColorTextureIndex;
    this_materialParameters.baseColorFactors = glm::vec4(1.f, 1.f, 1.f, 1.f);
    this_materialParameters.baseColorTexCoord = 0;
    this_materialParameters.alphaCutoff = 0.5f;

    this_materialParameters.normalTexture = defaultNormalTextureIndex;
    this_materialParameters.normalTexCoord = 0;
    this_materialParameters.normalScale = 0.f;

    materialsAbout.emplace_back(this_materialAbout);
    materialsParameters.emplace_back(this_materialParameters);
}

MaterialsOfPrimitives::~MaterialsOfPrimitives()
{
    device.destroy(descriptorSetLayout);
    device.destroy(descriptorPool);

    vma_allocator.destroyBuffer(materialParametersBuffer, materialParametersAllocation);
}

struct ColorTextureSpecs {
    const tinygltf::Image* image_ptr = nullptr;
    glTFsamplerWrap wrap_S = glTFsamplerWrap::repeat;
    glTFsamplerWrap wrap_T = glTFsamplerWrap::repeat;

    bool operator==(const ColorTextureSpecs& rhs) const {
        return image_ptr == rhs.image_ptr &&
            wrap_S == rhs.wrap_S &&
            wrap_T == rhs.wrap_T;
    }
};

template <>
struct std::hash<ColorTextureSpecs> {
    inline std::size_t operator()(const ColorTextureSpecs& rhs) const noexcept{
        size_t hash = 0;
        hash_combine( hash, std::hash<const tinygltf::Image*>{}(rhs.image_ptr));
        hash_combine( hash, std::hash<glTFsamplerWrap>{}(rhs.wrap_S));
        hash_combine( hash, std::hash<glTFsamplerWrap>{}(rhs.wrap_T));
        return hash;
    }
};

struct NormalTextureSpecs {
    const tinygltf::Image* image_ptr = nullptr;
    glTFsamplerWrap wrap_S = glTFsamplerWrap::repeat;
    glTFsamplerWrap wrap_T = glTFsamplerWrap::repeat;
    float scale = 1.f;

    bool operator==(const NormalTextureSpecs& rhs) const {
        return image_ptr == rhs.image_ptr &&
               wrap_S == rhs.wrap_S &&
               wrap_T == rhs.wrap_T &&
               scale == rhs.scale;
    }
};

template <>
struct std::hash<NormalTextureSpecs> {
    inline std::size_t operator()(const NormalTextureSpecs& rhs) const noexcept{
        size_t hash = 0;
        hash_combine( hash, std::hash<const tinygltf::Image*>{}(rhs.image_ptr));
        hash_combine( hash, std::hash<glTFsamplerWrap>{}(rhs.wrap_S));
        hash_combine( hash, std::hash<glTFsamplerWrap>{}(rhs.wrap_T));
        hash_combine( hash, std::hash<float>{}(rhs.scale));
        return hash;
    }
};


void MaterialsOfPrimitives::AddMaterialsOfModel(const tinygltf::Model& model, const std::string& model_folder)
{
    assert(!hasBeenFlashed);

    modelToMaterialIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&model), GetMaterialsCount());

    std::unordered_map<ColorTextureSpecs, size_t> colorTextureSpecsToTextureIndex_umap;
    std::unordered_map<NormalTextureSpecs, size_t> normalTextureSpecsToTextureIndex_umap;

    for (size_t this_material_index = 0; this_material_index != model.materials.size(); ++this_material_index) {
        const tinygltf::Material& this_material = model.materials[this_material_index];

        MaterialAbout this_materialAbout;
        MaterialParameters this_materialParameters = {};

        // materialAbout stuff
        if (this_material.doubleSided)
            this_materialAbout.twoSided = true;
        else
            this_materialAbout.twoSided = false;

        if (this_material.alphaMode == "BLEND") {
            this_materialAbout.transparent = true;
            this_materialAbout.masked = false;
            this_materialAbout.definitionStringPairs.emplace_back("IS_TRANSPARENT", "");
        } else if (this_material.alphaMode == "MASK") {
            this_materialAbout.transparent = false;
            this_materialAbout.masked = true;
            this_materialAbout.definitionStringPairs.emplace_back("IS_MASKED", "");
        } else {
            this_materialAbout.transparent = false;
            this_materialAbout.masked = false;
            this_materialAbout.definitionStringPairs.emplace_back("IS_OPAQUE", "");
        }

        {   // baseColorFactor
            glm::vec4 this_material_base_color_factors(static_cast<float>(this_material.pbrMetallicRoughness.baseColorFactor[0]),
                                                       static_cast<float>(this_material.pbrMetallicRoughness.baseColorFactor[1]),
                                                       static_cast<float>(this_material.pbrMetallicRoughness.baseColorFactor[2]),
                                                       static_cast<float>(this_material.pbrMetallicRoughness.baseColorFactor[3]));

            this_materialParameters.baseColorFactors = this_material_base_color_factors;
        }
        {   // alphaCutoff
            this_materialParameters.alphaCutoff = float(this_material.alphaCutoff);
        }
        {   // baseColorTexture
            if (this_material.pbrMetallicRoughness.baseColorTexture.index != -1) {
                const tinygltf::Texture& color_texture = model.textures[this_material.pbrMetallicRoughness.baseColorTexture.index];
                if(color_texture.source != -1) {
                    ColorTextureSpecs colorTextureSpecs = {};
                    colorTextureSpecs.image_ptr = &model.images[color_texture.source];
                    if(color_texture.sampler != -1) {
                        const tinygltf::Sampler& this_sampler = model.samplers[color_texture.sampler];
                        colorTextureSpecs.wrap_S = static_cast<glTFsamplerWrap>(this_sampler.wrapS);
                        colorTextureSpecs.wrap_T = static_cast<glTFsamplerWrap>(this_sampler.wrapT);
                    }

                    auto search = colorTextureSpecsToTextureIndex_umap.find(colorTextureSpecs);
                    if(search != colorTextureSpecsToTextureIndex_umap.end()) {
                        this_materialParameters.baseColorTexture = uint32_t(search->second);
                    } else {
                        LinearImage color_image = {*colorTextureSpecs.image_ptr,
                                                   (this_material.name.size() ? this_material.name : std::to_string(this_material_index)) + "_colorTexture",
                                                   model_folder,
                                                   colorTextureSpecs.wrap_S,
                                                   colorTextureSpecs.wrap_T};

                        color_image.RetrieveMipmaps(16, 16);
                        size_t texture_index = texturesOfMaterials_ptr->AddTextureAndMipmaps(color_image.GetMipmaps(),
                                                                                             vk::Format::eR8G8B8A8Srgb);

                        this_materialParameters.baseColorTexture = uint32_t(texture_index);
                        colorTextureSpecsToTextureIndex_umap.emplace(colorTextureSpecs, texture_index);

                        color_image.SaveMipmaps();
                    }
                } else {
                    this_materialParameters.baseColorTexture = defaultColorTextureIndex;
                }
            } else {
                this_materialParameters.baseColorTexture = defaultColorTextureIndex;
            }

            this_materialParameters.baseColorTexCoord = this_material.pbrMetallicRoughness.baseColorTexture.texCoord;
            this_materialAbout.color_texcooord = this_material.pbrMetallicRoughness.baseColorTexture.texCoord;
        }
        {   // normalTexture
            if (this_material.normalTexture.index != -1) {
                const tinygltf::Texture& normal_texture = model.textures[this_material.normalTexture.index];
                if(normal_texture.source != -1) {
                    NormalTextureSpecs normalTextureSpecs = {};
                    normalTextureSpecs.image_ptr = &model.images[normal_texture.source];
                    normalTextureSpecs.scale = float(this_material.normalTexture.scale);
                    if(normal_texture.sampler != -1) {
                        const tinygltf::Sampler& this_sampler = model.samplers[normal_texture.sampler];
                        normalTextureSpecs.wrap_S = static_cast<glTFsamplerWrap>(this_sampler.wrapS);
                        normalTextureSpecs.wrap_T = static_cast<glTFsamplerWrap>(this_sampler.wrapT);
                    }

                    auto search = normalTextureSpecsToTextureIndex_umap.find(normalTextureSpecs);
                    if(search != normalTextureSpecsToTextureIndex_umap.end()) {
                        this_materialParameters.normalTexture = uint32_t(search->second);
                        this_materialParameters.normalScale = normalTextureSpecs.scale;
                    } else {
                        NormalImage normal_image = {*normalTextureSpecs.image_ptr,
                                                    (this_material.name.size() ? this_material.name : std::to_string(this_material_index)) + "_normalTexture",
                                                    model_folder,
                                                    normalTextureSpecs.wrap_S,
                                                    normalTextureSpecs.wrap_T,
                                                    normalTextureSpecs.scale};

                        normal_image.RetrieveMipmaps(16, 16);
                        size_t texture_index = texturesOfMaterials_ptr->AddTextureAndMipmaps(normal_image.GetMipmaps(),
                                                                                             vk::Format::eA2R10G10B10UnormPack32);

                        this_materialParameters.normalTexture = uint32_t(texture_index);
                        this_materialParameters.normalScale = normalTextureSpecs.scale;

                        normal_image.SaveMipmaps();
                    }

                } else {
                    this_materialParameters.normalTexture = defaultNormalTextureIndex;
                    this_materialParameters.normalScale = 0.f;
                }
            } else {
                this_materialParameters.normalTexture = defaultNormalTextureIndex;
                this_materialParameters.normalScale = 0.f;
            }

            this_materialParameters.normalTexCoord = this_material.normalTexture.texCoord;
        }


        materialsAbout.emplace_back(this_materialAbout);
        materialsParameters.emplace_back(this_materialParameters);
    }

}

void MaterialsOfPrimitives::FlashDevice(std::pair<vk::Queue, uint32_t> queue)
{
    assert(!hasBeenFlashed);

    InformShadersSpecsAboutRanges(texturesOfMaterials_ptr->GetTexturesCount(), GetMaterialsCount());

    // Create and transfer to buffer
    size_t buffer_size_bytes = GetMaterialParametersBufferSize();
    {   // Create materialParametersBuffer
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = buffer_size_bytes;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, buffer_allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        materialParametersBuffer = createBuffer_result.value.first;
        materialParametersAllocation = createBuffer_result.value.second;
    }
    {   // Transfer data
        StagingBuffer staging_buffer(device, vma_allocator, buffer_size_bytes);
        std::byte *dst_ptr = staging_buffer.GetDstPtr();

        memcpy(dst_ptr, materialsParameters.data(), buffer_size_bytes);

        vk::CommandBuffer command_buffer = staging_buffer.BeginCommandRecord(queue);
        vk::Buffer copy_buffer = staging_buffer.GetBuffer();

        vk::BufferCopy copy_region;
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = buffer_size_bytes;
        command_buffer.copyBuffer(copy_buffer, materialParametersBuffer, 1, &copy_region);

        staging_buffer.EndAndSubmitCommands();

        hasBeenFlashed = true;
        materialsParameters.clear();
    }

    // Create and write descriptor set
    {   // Create descriptor set
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eStorageBuffer, 1);
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eCombinedImageSampler, uint32_t(texturesOfMaterials_ptr->GetTexturesCount()));
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({},1,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;

        std::vector<vk::DescriptorSetLayoutBinding> descriptor_set_layout_bindings;
        {
            vk::DescriptorSetLayoutBinding buffer_binding;
            buffer_binding.binding = 0;
            buffer_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
            buffer_binding.descriptorCount = 1;
            buffer_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            descriptor_set_layout_bindings.emplace_back(buffer_binding);
        }
        {
            vk::DescriptorSetLayoutBinding textures_binding;
            textures_binding.binding = 1;
            textures_binding.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            textures_binding.descriptorCount = uint32_t(texturesOfMaterials_ptr->GetTexturesCount());
            textures_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            descriptor_set_layout_bindings.emplace_back(textures_binding);
        }


        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({},descriptor_set_layout_bindings);
        descriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, 1, &descriptorSetLayout);
        descriptorSet = device.allocateDescriptorSets(descriptor_set_allocate_info).value[0];
    }
    {   // Write descriptor set
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;

        std::unique_ptr<vk::DescriptorBufferInfo> descriptor_buffer_info_uptr;
        {
            vk::DescriptorBufferInfo descriptor_buffer_info;
            descriptor_buffer_info.buffer = materialParametersBuffer;
            descriptor_buffer_info.offset = 0;
            descriptor_buffer_info.range = VK_WHOLE_SIZE;
            descriptor_buffer_info_uptr = std::make_unique<vk::DescriptorBufferInfo>(descriptor_buffer_info);

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = descriptorSet;
            write_descriptor_set.dstBinding = 0;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = 1;
            write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
            write_descriptor_set.pBufferInfo = descriptor_buffer_info_uptr.get();

            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        std::vector<vk::DescriptorImageInfo> descriptor_combine_infos;
        {
            for (const auto &image_sampler_pair: texturesOfMaterials_ptr->GetTextures()) {
                vk::DescriptorImageInfo descriptor_image_info;
                descriptor_image_info.imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
                descriptor_image_info.imageView = image_sampler_pair.first;
                descriptor_image_info.sampler = image_sampler_pair.second;

                descriptor_combine_infos.emplace_back(descriptor_image_info);
            }

            vk::WriteDescriptorSet write_descriptor_set;
            write_descriptor_set.dstSet = descriptorSet;
            write_descriptor_set.dstBinding = 1;
            write_descriptor_set.dstArrayElement = 0;
            write_descriptor_set.descriptorCount = uint32_t(descriptor_combine_infos.size());
            write_descriptor_set.descriptorType = vk::DescriptorType::eCombinedImageSampler;
            write_descriptor_set.pImageInfo = descriptor_combine_infos.data();

            writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        device.updateDescriptorSets(writes_descriptor_set, {});
    }
}

size_t MaterialsOfPrimitives::GetMaterialIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToMaterialIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));
    assert(search != modelToMaterialIndexOffset_umap.end());

    return search->second;
}

void MaterialsOfPrimitives::InformShadersSpecsAboutRanges(size_t textures_count, size_t materials_count)
{
    for (MaterialAbout& this_material : materialsAbout)
    {
        this_material.definitionStringPairs.emplace_back(std::make_pair("MATERIALS_PARAMETERS_COUNT", std::to_string(materials_count)));
        this_material.definitionStringPairs.emplace_back(std::make_pair("TEXTURES_COUNT", std::to_string(textures_count)));
    }
}

size_t MaterialsOfPrimitives::GetMaterialParametersBufferSize() const
{
    return GetMaterialsCount() * sizeof(MaterialParameters);
}




