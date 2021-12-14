#pragma once

#include <unordered_map>

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "glTFenum.h"
#include "const_maps.h"
#include "hash_combine.h"
#include "Graphics/ImagesInterfaces.h"

struct SamplerSpecs {
    glTFsamplerWrap wrap_S;
    glTFsamplerWrap wrap_T;

    bool operator==(const SamplerSpecs& rhs) const {
        return wrap_S == rhs.wrap_S && wrap_T == rhs.wrap_T;
    }
};

template <>
struct std::hash<SamplerSpecs> {
    inline std::size_t operator()(const SamplerSpecs& rhs) const noexcept {
        size_t hash = 0;
        hash_combine( hash, std::hash<glTFsamplerWrap>{}(rhs.wrap_S));
        hash_combine( hash, std::hash<glTFsamplerWrap>{}(rhs.wrap_T));
        return hash;
    }
};

class TexturesOfMaterials {
public:
    TexturesOfMaterials(vk::Device device,
                        vma::Allocator vma_allocator,
                        std::pair<vk::Queue, uint32_t> graphics_queue);

    ~TexturesOfMaterials();

    size_t AddTextureAndMipmaps(const std::vector<ImageData>& images_data, vk::Format format);
    const std::vector<std::pair<vk::ImageView, vk::Sampler>>& GetTextures() const {return textures;};
    size_t GetTexturesCount() const {return textures.size();}

private:
    vk::Sampler GetSampler(SamplerSpecs samplerSpecs);

    static std::vector<std::byte> GetImageFromImageData(const ImageData& image_data, vk::Format format);

private:
    std::unordered_map<SamplerSpecs, vk::Sampler> samplerSpecToSampler_umap;
    std::vector<std::pair<vk::ImageView, vk::Sampler>> textures;

    std::vector<std::pair<vk::Image, vma::Allocation>> vkImagesAndAllocations;

    vk::Device device;
    vma::Allocator vma_allocator;
    std::pair<vk::Queue, uint32_t> transferQueue;
};
