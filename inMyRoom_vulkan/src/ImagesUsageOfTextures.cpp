#include "ImagesUsageOfTextures.h"

void ImagesUsageOfTextures::AddImagesUsageOfModel(const tinygltf::Model& in_model)
{
    for (const tinygltf::Material& this_material : in_model.materials)
    {
        {
            auto search = this_material.values.find("baseColorTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                const tinygltf::Image& image_being_used = in_model.images[image_index];

                RegistImage(image_being_used, ImageUsage::baseColor);
            }
        }

        {
            auto search = this_material.values.find("metallicTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                const tinygltf::Image& image_being_used = in_model.images[image_index];

                RegistImage(image_being_used, ImageUsage::metallic);
            }
        }

        {
            auto search = this_material.values.find("roughnessTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                const tinygltf::Image& image_being_used = in_model.images[image_index];

                RegistImage(image_being_used, ImageUsage::roughness);
            }
        }

        {
            auto search = this_material.additionalValues.find("normalTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                const tinygltf::Image& image_being_used = in_model.images[image_index];

                RegistImage(image_being_used, ImageUsage::normal);
            }
        }

        {
            auto search = this_material.additionalValues.find("occlusionTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                const tinygltf::Image& image_being_used = in_model.images[image_index];

                RegistImage(image_being_used, ImageUsage::occlusion);
            }
        }

        {
            auto search = this_material.additionalValues.find("emissiveTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                const tinygltf::Image& image_being_used = in_model.images[image_index];

                RegistImage(image_being_used, ImageUsage::emissive);
            }
        }
    }

}

void ImagesUsageOfTextures::RegistImage(const tinygltf::Image& in_image, ImageUsage in_imageUsage)
{
    imagesUsage_umap.emplace(const_cast<tinygltf::Image*>(&in_image), in_imageUsage);
}

ImageUsage ImagesUsageOfTextures::GetImageUsage(const tinygltf::Image& in_image) const
{
    auto search = imagesUsage_umap.find(const_cast<tinygltf::Image*>(&in_image));

    if (search != imagesUsage_umap.end())
    {
        return search->second;
    }
    else
    {
        return ImageUsage::undefined;
    }
}