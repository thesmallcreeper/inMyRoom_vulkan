#include "ImagesUsageOfTextures.h"

void ImagesUsageOfTextures::registImagesOfModel(tinygltf::Model& in_model)
{
    for (tinygltf::Material& this_material : in_model.materials)
    {
        {
            auto search = this_material.values.find("baseColorTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                tinygltf::Image& image_being_used = in_model.images[image_index];

                registImage(image_being_used, ImageUsage::baseColor);
            }
        }

        {
            auto search = this_material.values.find("metallicTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                tinygltf::Image& image_being_used = in_model.images[image_index];

                registImage(image_being_used, ImageUsage::metallic);
            }
        }

        {
            auto search = this_material.values.find("roughnessTexture");

            if (search != this_material.values.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                tinygltf::Image& image_being_used = in_model.images[image_index];

                registImage(image_being_used, ImageUsage::roughness);
            }
        }

        {
            auto search = this_material.additionalValues.find("normalTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                tinygltf::Image& image_being_used = in_model.images[image_index];

                registImage(image_being_used, ImageUsage::normal);
            }
        }

        {
            auto search = this_material.additionalValues.find("occlusionTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                tinygltf::Image& image_being_used = in_model.images[image_index];

                registImage(image_being_used, ImageUsage::occlusion);
            }
        }

        {
            auto search = this_material.additionalValues.find("emissiveTexture");

            if (search != this_material.additionalValues.end())
            {
                auto texture_index = search->second.TextureIndex();
                auto image_index = in_model.textures[texture_index].source;
                tinygltf::Image& image_being_used = in_model.images[image_index];

                registImage(image_being_used, ImageUsage::emissive);
            }
        }
    }

}

void ImagesUsageOfTextures::registImage(tinygltf::Image& in_image, ImageUsage in_imageUsage)
{
    std::string image_uri = ImageInfoToString(in_image);
    imagesUsage_umap.emplace(image_uri, in_imageUsage);
}

ImageUsage ImagesUsageOfTextures::getImageUsage(tinygltf::Image& in_image) const
{
    std::string image_uri = ImageInfoToString(in_image);
    auto search = imagesUsage_umap.find(image_uri);

    if (search != imagesUsage_umap.end())
    {
        return search->second;
    }
    else
    {
        return ImageUsage::undefined;
    }
}

std::string ImagesUsageOfTextures::ImageInfoToString(tinygltf::Image& in_image) //static
{
    std::string return_string;

    return_string += in_image.name;
    return_string += std::string(in_image.image.begin(), in_image.image.end());
    return_string += in_image.mimeType;
    return_string += in_image.uri;

    return return_string;
}