#include "Meshes/ImagesAboutOfTextures.h"

#include <array>
#include <string>

void ImagesAboutOfTextures::AddImagesUsageOfModel(const tinygltf::Model& in_model)
{
    for (const tinygltf::Material& this_material : in_model.materials)
    {
        std::array<ImageAbout, 6> one_ImageAbout_for_every_ImageMap;

        {
            auto search = this_material.values.find("baseColorTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                auto sampler_index = in_model.textures[texture_index].sampler;

                const tinygltf::Image& image_being_used = in_model.images[image_index];
                for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                    this_ImageAbout.sibling_baseColor_image = &image_being_used;

                one_ImageAbout_for_every_ImageMap[0].map = ImageMap::baseColor;
                if (sampler_index != -1)
                {
                    one_ImageAbout_for_every_ImageMap[0].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                    one_ImageAbout_for_every_ImageMap[0].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
                }
                
            }
        }

        {
            auto search = this_material.values.find("metallicTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                auto sampler_index = in_model.textures[texture_index].sampler;

                const tinygltf::Image& image_being_used = in_model.images[image_index];
                for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                    this_ImageAbout.sibling_baseColor_image = &image_being_used;

                one_ImageAbout_for_every_ImageMap[1].map = ImageMap::metallic;
                if (sampler_index != -1)
                {
                    one_ImageAbout_for_every_ImageMap[1].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                    one_ImageAbout_for_every_ImageMap[1].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
                }
            }
        }

        {
            auto search = this_material.values.find("roughnessTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                auto sampler_index = in_model.textures[texture_index].sampler;

                const tinygltf::Image& image_being_used = in_model.images[image_index];
                for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                    this_ImageAbout.sibling_baseColor_image = &image_being_used;

                one_ImageAbout_for_every_ImageMap[2].map = ImageMap::roughness;
                if (sampler_index != -1)
                {
                    one_ImageAbout_for_every_ImageMap[2].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                    one_ImageAbout_for_every_ImageMap[2].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
                }
            }
        }

        {
            auto search = this_material.additionalValues.find("normalTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                auto sampler_index = in_model.textures[texture_index].sampler;

                const tinygltf::Image& image_being_used = in_model.images[image_index];
                for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                    this_ImageAbout.sibling_baseColor_image = &image_being_used;

                one_ImageAbout_for_every_ImageMap[3].map = ImageMap::normal;
                if (sampler_index != -1)
                {
                    one_ImageAbout_for_every_ImageMap[3].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                    one_ImageAbout_for_every_ImageMap[3].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
                }
            }
        }

        {
            auto search = this_material.additionalValues.find("occlusionTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                auto sampler_index = in_model.textures[texture_index].sampler;

                const tinygltf::Image& image_being_used = in_model.images[image_index];
                for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                    this_ImageAbout.sibling_baseColor_image = &image_being_used;

                one_ImageAbout_for_every_ImageMap[4].map = ImageMap::occlusion;
                if (sampler_index != -1)
                {
                    one_ImageAbout_for_every_ImageMap[4].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                    one_ImageAbout_for_every_ImageMap[4].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
                }
            }
        }

        {
            auto search = this_material.additionalValues.find("emissiveTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                auto sampler_index = in_model.textures[texture_index].sampler;

                const tinygltf::Image& image_being_used = in_model.images[image_index];
                for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
                    this_ImageAbout.sibling_baseColor_image = &image_being_used;

                one_ImageAbout_for_every_ImageMap[5].map = ImageMap::emissive;
                if (sampler_index != -1)
                {
                    one_ImageAbout_for_every_ImageMap[5].wrapS = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapS);
                    one_ImageAbout_for_every_ImageMap[5].wrapT = static_cast<glTFsamplerWrap>(in_model.samplers[sampler_index].wrapT);
                }
            }
        }

        for (auto& this_ImageAbout : one_ImageAbout_for_every_ImageMap)
            if (this_ImageAbout.map != ImageMap::undefined)
                RegistImage(this_ImageAbout);
    }

}

void ImagesAboutOfTextures::RegistImage(ImageAbout in_imageAbout)
{
    const tinygltf::Image* image_ptr;

    switch (in_imageAbout.map)
    {
        case ImageMap::metallic:
            image_ptr = in_imageAbout.sibling_metallic_image;
            break;
        case ImageMap::baseColor:
            image_ptr = in_imageAbout.sibling_baseColor_image;
            break;
        case ImageMap::roughness:
            image_ptr = in_imageAbout.sibling_roughness_image;
            break;
        case ImageMap::normal:
            image_ptr = in_imageAbout.sibling_normal_image;
            break;
        case ImageMap::occlusion:
            image_ptr = in_imageAbout.sibling_occlusion_image;
            break;
        case ImageMap::emissive:
            image_ptr = in_imageAbout.sibling_emissive_image;
            break;
        default:
            assert(0);
    }
    imagesUsage_umap.try_emplace(const_cast<tinygltf::Image*>(image_ptr), in_imageAbout);
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