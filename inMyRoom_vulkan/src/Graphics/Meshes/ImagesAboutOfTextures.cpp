#include "Graphics/Meshes/ImagesAboutOfTextures.h"

#include <array>
#include <string>

void ImagesAboutOfTextures::AddImagesUsageOfModel(const tinygltf::Model& in_model)
{
    for (const tinygltf::Material& this_material : in_model.materials)
    {
        std::array<ImageAbout, 5> one_ImageAbout_for_every_ImageMap;

        if (this_material.pbrMetallicRoughness.baseColorTexture.index != -1)
        {
            int texture_index = this_material.pbrMetallicRoughness.baseColorTexture.index;
            int image_index = in_model.textures[texture_index].source;
            int sampler_index = in_model.textures[texture_index].sampler;

            const tinygltf::Image& image_being_used = in_model.images[image_index];
            for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                this_ImageAbout.sibling_baseColor_image_ptr = &image_being_used;

            one_ImageAbout_for_every_ImageMap[0].map = ImageMap::baseColor;
            if (sampler_index != -1)
            {
                one_ImageAbout_for_every_ImageMap[0].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                one_ImageAbout_for_every_ImageMap[0].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
            }
                
            one_ImageAbout_for_every_ImageMap[0].material_example_ptr = &this_material;
        }

        if (this_material.pbrMetallicRoughness.metallicRoughnessTexture.index != -1)
        {
            int texture_index = this_material.pbrMetallicRoughness.metallicRoughnessTexture.index;
            int image_index = in_model.textures[texture_index].source;
            int sampler_index = in_model.textures[texture_index].sampler;

            const tinygltf::Image& image_being_used = in_model.images[image_index];
            for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                this_ImageAbout.sibling_metallicRoughness_image_ptr = &image_being_used;

            one_ImageAbout_for_every_ImageMap[1].map = ImageMap::metallicRoughness;
            if (sampler_index != -1)
            {
                one_ImageAbout_for_every_ImageMap[1].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                one_ImageAbout_for_every_ImageMap[1].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
            }

            one_ImageAbout_for_every_ImageMap[1].material_example_ptr = &this_material;
        }

        if (this_material.normalTexture.index != -1)
        {
            int texture_index = this_material.normalTexture.index;
            int image_index = in_model.textures[texture_index].source;
            int sampler_index = in_model.textures[texture_index].sampler;

            const tinygltf::Image& image_being_used = in_model.images[image_index];
            for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                this_ImageAbout.sibling_normal_image_ptr = &image_being_used;

            one_ImageAbout_for_every_ImageMap[2].map = ImageMap::normal;
            if (sampler_index != -1)
            {
                one_ImageAbout_for_every_ImageMap[2].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                one_ImageAbout_for_every_ImageMap[2].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
            }

            one_ImageAbout_for_every_ImageMap[2].material_example_ptr = &this_material;
        }
        if (this_material.occlusionTexture.index != -1)
        {
            int texture_index = this_material.occlusionTexture.index;
            int image_index = in_model.textures[texture_index].source;
            int sampler_index = in_model.textures[texture_index].sampler;

            const tinygltf::Image& image_being_used = in_model.images[image_index];
            for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                this_ImageAbout.sibling_occlusion_image_ptr = &image_being_used;

            one_ImageAbout_for_every_ImageMap[3].map = ImageMap::occlusion;
            if (sampler_index != -1)
            {
                one_ImageAbout_for_every_ImageMap[3].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                one_ImageAbout_for_every_ImageMap[3].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
            }

            one_ImageAbout_for_every_ImageMap[3].material_example_ptr = &this_material;
        }
        if (this_material.emissiveTexture.index != -1)
        {
            int texture_index = this_material.emissiveTexture.index;
            int image_index = in_model.textures[texture_index].source;
            int sampler_index = in_model.textures[texture_index].sampler;

            const tinygltf::Image& image_being_used = in_model.images[image_index];
            for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                this_ImageAbout.sibling_emissive_image_ptr = &image_being_used;

            one_ImageAbout_for_every_ImageMap[4].map = ImageMap::emissive;
            if (sampler_index != -1)
            {
                one_ImageAbout_for_every_ImageMap[4].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                one_ImageAbout_for_every_ImageMap[4].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
            }

            one_ImageAbout_for_every_ImageMap[4].material_example_ptr = &this_material;
        }

        for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
            if (this_ImageAbout.map != ImageMap::undefined)
            {
                RegistImage(this_ImageAbout);
            }
                
    }

}

void ImagesAboutOfTextures::RegistImage(ImageAbout in_imageAbout)
{
    const tinygltf::Image* image_ptr;

    switch (in_imageAbout.map)
    {
        case ImageMap::baseColor:
            image_ptr = in_imageAbout.sibling_baseColor_image_ptr;
            break;
        case ImageMap::metallicRoughness:
            image_ptr = in_imageAbout.sibling_metallicRoughness_image_ptr;
            break;
        case ImageMap::normal:
            image_ptr = in_imageAbout.sibling_normal_image_ptr;
            break;
        case ImageMap::occlusion:
            image_ptr = in_imageAbout.sibling_occlusion_image_ptr;
            break;
        case ImageMap::emissive:
            image_ptr = in_imageAbout.sibling_emissive_image_ptr;
            break;
        default:
            assert(0);
    }

    auto search = imagesUsage_umap.find(const_cast<tinygltf::Image*>(image_ptr));
    if (search == imagesUsage_umap.end())
    {
        imagesUsage_umap.emplace(const_cast<tinygltf::Image*>(image_ptr), in_imageAbout);
    }
    else
    {
        ImageAbout this_old_image_regist = search->second;
        in_imageAbout.map = in_imageAbout.map | this_old_image_regist.map;

        if (!in_imageAbout.sibling_baseColor_image_ptr) in_imageAbout.sibling_baseColor_image_ptr = this_old_image_regist.sibling_baseColor_image_ptr;
        if (!in_imageAbout.sibling_metallicRoughness_image_ptr) in_imageAbout.sibling_metallicRoughness_image_ptr = this_old_image_regist.sibling_metallicRoughness_image_ptr;
        if (!in_imageAbout.sibling_normal_image_ptr) in_imageAbout.sibling_normal_image_ptr = this_old_image_regist.sibling_normal_image_ptr;
        if (!in_imageAbout.sibling_occlusion_image_ptr) in_imageAbout.sibling_occlusion_image_ptr = this_old_image_regist.sibling_occlusion_image_ptr;
        if (!in_imageAbout.sibling_emissive_image_ptr) in_imageAbout.sibling_emissive_image_ptr = this_old_image_regist.sibling_emissive_image_ptr;

        imagesUsage_umap.emplace(const_cast<tinygltf::Image*>(image_ptr), in_imageAbout);
    }
}

ImageAbout ImagesAboutOfTextures::GetImageAbout(const tinygltf::Image& in_image) const
{
    auto search = imagesUsage_umap.find(const_cast<tinygltf::Image*>(&in_image));

    if (search != imagesUsage_umap.end())
    {
        return search->second;
    }
    else
    {
        return ImageAbout();
    }
}