#include "TexturesImagesUsage.h"

TexturesImagesUsage::TexturesImagesUsage(tinygltf::Model& in_model)
{
    std::vector<ImageUsage> images_usage(in_model.images.size(), ImageUsage::undefined);

    for (tinygltf::Material& this_material : in_model.materials)
    {
        {
            auto search = this_material.values.find("baseColorTexture");

            if (search != this_material.values.end())
            {
                auto this_texture_index = search->second.TextureIndex();

                auto this_image_index = in_model.textures[this_texture_index].source;

                images_usage[this_image_index] = ImageUsage::baseColor;
            }
        }

        {
            auto search = this_material.values.find("metallicTexture");

            if (search != this_material.values.end())
            {
                auto this_texture_index = search->second.TextureIndex();

                auto this_image_index = in_model.textures[this_texture_index].source;

                images_usage[this_image_index] = ImageUsage::metallic;
            }
        }

        {
            auto search = this_material.values.find("roughnessTexture");

            if (search != this_material.values.end())
            {
                auto this_texture_index = search->second.TextureIndex();

                auto this_image_index = in_model.textures[this_texture_index].source;

                images_usage[this_image_index] = ImageUsage::roughness;
            }
        }

        {
            auto search = this_material.additionalValues.find("normalTexture");

            if (search != this_material.additionalValues.end())
            {
                auto this_texture_index = search->second.TextureIndex();

                auto this_image_index = in_model.textures[this_texture_index].source;

                images_usage[this_image_index] = ImageUsage::normal;
            }
        }

        {
            auto search = this_material.additionalValues.find("occlusionTexture");

            if (search != this_material.additionalValues.end())
            {
                auto this_texture_index = search->second.TextureIndex();

                auto this_image_index = in_model.textures[this_texture_index].source;

                images_usage[this_image_index] = ImageUsage::occlusion;
            }
        }

        {
            auto search = this_material.additionalValues.find("emissiveTexture");

            if (search != this_material.additionalValues.end())
            {
                auto this_texture_index = search->second.TextureIndex();

                auto this_image_index = in_model.textures[this_texture_index].source;

                images_usage[this_image_index] = ImageUsage::emissive;
            }
        }
    }

    imagesUsage.swap(images_usage);
}

TexturesImagesUsage::~TexturesImagesUsage()
{
    imagesUsage.clear();
}