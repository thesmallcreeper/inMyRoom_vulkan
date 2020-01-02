#pragma once

#ifdef __GNUC__
#if __GNUC__ < 8
#define FILESYSTEM_IS_EXPERIMENTAL
#else
#endif
#endif

#include <unordered_map>

#include "hash_combine.h"

#include "wrappers/device.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/sampler.h"

#include "tiny_gltf.h"

#include "glTFenum.h"
#include "const_maps.h"

#include "Graphics/MipmapsGenerator.h"


struct SamplerSpecs
{
    glTFsamplerMagFilter magFilter;
    glTFsamplerMinFilter minFilter;
    glTFsamplerWrap wrapS;
    glTFsamplerWrap wrapT;
};

namespace std
{
    template <>
    struct hash<SamplerSpecs>
    {
        std::size_t operator()(const SamplerSpecs& in_samplerSpecs) const
        {
            std::size_t result = 0;
            hash_combine(result, in_samplerSpecs.magFilter);
            hash_combine(result, in_samplerSpecs.minFilter);
            hash_combine(result, in_samplerSpecs.wrapS);
            hash_combine(result, in_samplerSpecs.wrapT);
            return result;
        }
    };

    template <>
    struct equal_to<SamplerSpecs>
    {
        bool operator()(const SamplerSpecs& lhs, const SamplerSpecs& rhs) const
        {
            bool isEqual = (lhs.magFilter == rhs.magFilter) &&
                           (lhs.minFilter == rhs.minFilter) &&
                           (lhs.wrapS == rhs.wrapS) &&
                           (lhs.wrapT == rhs.wrapT);

            return isEqual;
        }
    };
}

struct TextureInfo
{
    size_t imageIndex;
    size_t samplerIndex;
};

class TexturesOfMaterials
{
public: // functions
    TexturesOfMaterials(bool use_mipmaps,
                        MipmapsGenerator* in_mipmapsGenerator_ptr,
                        Anvil::BaseDevice* const in_device_ptr);
    ~TexturesOfMaterials();

    void AddTexturesOfModel(const tinygltf::Model& in_model, const std::string in_imagesFolder);
    size_t GetTextureIndexOffsetOfModel(const tinygltf::Model& in_model);

    Anvil::ImageView* GetImageView(size_t in_texture_index);
    Anvil::Sampler* GetSampler(size_t in_texture_index);
     
private: // functions

    size_t GetSamplerIndex(SamplerSpecs in_samplerSpecs);
 

private: // data
    const bool useMipmaps;

    size_t imagesSoFar;
    size_t texturesSoFar;

    std::vector<TextureInfo> texturesInfos;

    std::vector<Anvil::ImageUniquePtr> images_uptrs;
    std::vector<Anvil::ImageViewUniquePtr> imagesViews_upts;
    std::vector<Anvil::SamplerUniquePtr> samplers_uptrs;

    std::unordered_map<SamplerSpecs, size_t> samplerSpecsToSamplerIndex_umap;

    std::unordered_map<tinygltf::Model*, size_t> modelToTextureIndexOffset_umap;

    const Anvil::Format image_preferred_compressed_format = Anvil::Format::BC7_SRGB_BLOCK;

    MipmapsGenerator* mipmapsGenerator_ptr;

    Anvil::BaseDevice* const device_ptr;
};
