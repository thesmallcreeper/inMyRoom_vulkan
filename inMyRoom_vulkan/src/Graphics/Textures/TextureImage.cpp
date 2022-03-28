#include "Graphics/Textures/TextureImage.h"

#include <filesystem>
#include <iostream>

#include "stb_image.h"
#include "stb_image_write.h"

TextureImage::TextureImage(const tinygltf::Image* gltf_image_ptr, std::string identifier_string, std::string model_folder,
                           glTFsamplerWrap in_wrap_S, glTFsamplerWrap in_wrap_T,
                           bool in_sRGB, bool in_saveAs16bit)
        : glTFimage_ptr(gltf_image_ptr),
          identifierString(std::move(identifier_string)),
          modelFolder(std::move(model_folder)),
          wrap_S(in_wrap_S),
          wrap_T(in_wrap_T),
          sRGBifPossible(in_sRGB),
          saveAs16bit(in_saveAs16bit)
{
}

void TextureImage::RetrieveMipmaps(size_t min_x, size_t min_y)
{
    assert(!glTFimage_ptr || glTFimage_ptr->width != -1);
    assert(!glTFimage_ptr || glTFimage_ptr->height != -1);

    size_t this_mipmap_width = 1;
    size_t this_mipmap_height = 1;

    size_t this_mipmap_factor = 1;
    size_t this_mipmap_level = 0;

    // Get mipmaps
    do {
        auto mipmap_opt = LoadMipmapFromDisk(this_mipmap_level);
        if (mipmap_opt) {
            imagesData.emplace_back(mipmap_opt.value());
        } else {
            std::cout << "---Creating mipmap of: " + identifierString + " , level = " + std::to_string(this_mipmap_level) + "\n";
            if (this_mipmap_level == 0 && glTFimage_ptr == nullptr) {
                ImageData this_image_data(0, 0, 0, wrap_S, wrap_T);
                imagesData.emplace_back(CreateMipmap(this_image_data, 0));
            } else if (this_mipmap_level == 0) {
                int width = glTFimage_ptr->width;
                int height = glTFimage_ptr->height;
                int componentsCount = (glTFimage_ptr->component == 3) ? 4 : glTFimage_ptr->component;

                ImageData this_image_data(width, height, componentsCount, wrap_S, wrap_T);
                if (glTFimage_ptr->bits == 8) {
                    std::vector<uint8_t> buffer;
                    for (size_t i = 0; i != glTFimage_ptr->image.size(); ++i) {
                        buffer.emplace_back(glTFimage_ptr->image[i]);
                        if (i % 3 == 2 && glTFimage_ptr->component == 3) {
                            buffer.emplace_back(std::numeric_limits<uint8_t>::max());
                        }
                    }
                    this_image_data.SetImage(buffer, sRGBifPossible);
                } else if (glTFimage_ptr->bits == 16) {
                    std::vector<uint16_t> buffer;
                    auto image_ptr = reinterpret_cast<const uint16_t*>(glTFimage_ptr->image.data());
                    for (size_t i = 0; i != glTFimage_ptr->image.size() / 2; ++i) {
                        buffer.emplace_back(image_ptr[i]);
                        if (i % 3 == 2 && glTFimage_ptr->component == 3) {
                            buffer.emplace_back(std::numeric_limits<uint16_t>::max());
                        }
                    }
                    this_image_data.SetImage(buffer);
                }

                imagesData.emplace_back(CreateMipmap(this_image_data, 1));
            } else {
                imagesData.emplace_back(CreateMipmap(imagesData[this_mipmap_level - 1], 2));
            }
        }

        this_mipmap_width = imagesData.back().GetWidth() / 2;
        this_mipmap_height = imagesData.back().GetHeight() / 2;
        this_mipmap_factor *= 2;
        ++this_mipmap_level;
    } while (this_mipmap_width >= min_x && this_mipmap_height >= min_y);
}

std::optional<ImageData> TextureImage::LoadMipmapFromDisk(size_t level)
{
    std::string path_to_mipmap = modelFolder + "/mipmaps/" + identifierString + "/mipmap_" + std::to_string(level) + ".png";
    if (std::filesystem::exists(path_to_mipmap)) {
        bool _16bit_channel = stbi_is_16_bit(path_to_mipmap.c_str());
        int width = -1;
        int height = -1;
        int components = 0;

        stbi_info(path_to_mipmap.c_str(), &width, &height, &components);
        if (components == 3) components = 4;

        ImageData return_imageData(width, height, components, wrap_S, wrap_T);
        if (_16bit_channel) {
            uint16_t* data = stbi_load_16(path_to_mipmap.c_str(), &width, &height, &components, components);

            std::vector<uint16_t> data_vec;
            std::copy(data, data + width*height*components, std::back_inserter(data_vec));
            stbi_image_free(data);

            return_imageData.SetImage(data_vec);
        } else {
            uint8_t* data = stbi_load(path_to_mipmap.c_str(), &width, &height, &components, components);

            std::vector<uint8_t> data_vec;
            std::copy(data, data + width*height*components, std::back_inserter(data_vec));
            stbi_image_free(data);

            return_imageData.SetImage(data_vec, sRGBifPossible);
        }

        return return_imageData;
    } else {
        return std::nullopt;
    }
}

void TextureImage::SaveMipmaps() const
{
    for(size_t level = 0; level != imagesData.size(); ++level) {
        SaveMipmapToDisk(level);
    }
}

void TextureImage::SaveMipmapToDisk(size_t level) const
{
    std::string path_to_mipmaps_folder = modelFolder + "/mipmaps";
    if (not std::filesystem::exists(path_to_mipmaps_folder))
        std::filesystem::create_directory(path_to_mipmaps_folder);

    std::string path_to_mipmap_texture_folder = path_to_mipmaps_folder + "/" + identifierString;
    if (not std::filesystem::exists(path_to_mipmap_texture_folder))
        std::filesystem::create_directory(path_to_mipmap_texture_folder);

    std::string path_to_this_mipmap = path_to_mipmap_texture_folder + "/mipmap_" + std::to_string(level) + ".png";
    if (not std::filesystem::exists(path_to_this_mipmap)) {
        int width = int(imagesData[level].GetWidth());
        int height = int(imagesData[level].GetHeight());
        int components = int(imagesData[level].GetComponentsCount());

        std::vector<std::byte> data;
        int bytes_per_texel = 0;
        if (saveAs16bit) {
            data = imagesData[level].GetImage16BitPerChannel();
            bytes_per_texel = int(imagesData[level].GetComponentsCount()) * 2;
            stbi_write_png_16(path_to_this_mipmap.c_str(), width, height, components, data.data(), bytes_per_texel * width);
        } else {
            data = imagesData[level].GetImage8BitPerChannel(sRGBifPossible);
            bytes_per_texel = int(imagesData[level].GetComponentsCount());
            stbi_write_png(path_to_this_mipmap.c_str(), width, height, components, data.data(), bytes_per_texel * width);
        }
    }
}

float GaussianFilterFactor(float x, float y, float sigma) {
    float r_squared = x * x + y * y;
    float var = sigma * sigma;
    return std::exp(- r_squared / (2 * var));
}
