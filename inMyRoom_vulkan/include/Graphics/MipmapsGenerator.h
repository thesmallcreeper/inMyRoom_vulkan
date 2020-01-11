#pragma once

#ifdef __GNUC__
#if __GNUC__ < 8
#define FILESYSTEM_IS_EXPERIMENTAL
#else
#endif
#endif

#ifdef FILESYSTEM_IS_EXPERIMENTAL
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#else
#include <filesystem>
namespace fs = std::filesystem;
#endif

#include "wrappers/device.h"
#include "wrappers/descriptor_set_group.h"
#include "wrappers/image.h"
#include "wrappers/image_view.h"
#include "wrappers/framebuffer.h"
#include "wrappers/render_pass.h"
#include "wrappers/sampler.h"
#include "wrappers/command_buffer.h"
#include "wrappers/command_pool.h"
#include "wrappers/semaphore.h"
#include "wrappers/fence.h"
#include "wrappers/queue.h"
#include "wrappers/buffer.h"
#include "wrappers/memory_block.h"

#include "tiny_gltf.h"
#include "const_maps.h"

#include "Graphics/PipelinesFactory.h"
#include "Graphics/ShadersSetsFamiliesCache.h"
#include "Graphics/Meshes/ImagesAboutOfTextures.h"

struct MipmapInfo
{
    Anvil::Format image_vulkan_format;
    size_t size;
    uint32_t pitch;
    uint32_t width;
    uint32_t height;
    uint32_t compCount;
    std::unique_ptr<uint8_t[]> data_uptr;
};

// Rule 1: occlution + metallicRoughness = occlutionMetallicRoughtness
// Rule 2: normal (X-Y) -> normal (X-Y-Z)
class MipmapsGenerator
{
public:
    MipmapsGenerator(PipelinesFactory* pipelinesFactory_ptr,
                     ShadersSetsFamiliesCache* in_shadersSetsFamiliesCache_ptr,
                     ImagesAboutOfTextures* in_imagesAboutOfTextures_ptr,
                     std::string in_16bitTo8bit_shadername,
                     std::string baseColor_shadername,
                     std::string occlusionMetallicRoughness_shadername,
                     std::string normal_shadername,
                     std::string emissive_shadername,
                     Anvil::PrimaryCommandBuffer* const in_cmd_buffer_ptr,
                     Anvil::BaseDevice* const in_device_ptr);
    ~MipmapsGenerator();

    void Reset();
    void BindNewImage(const tinygltf::Image& in_image ,const std::string in_imagesFolder);

    MipmapInfo GetOriginal();
    MipmapInfo GetOriginalInfoOnly();
    MipmapInfo GetMipmap(size_t mipmap_level);
    size_t GetMipmaps_levels_over_4x4();

private: //functions
    MipmapInfo LoadImageFileFromDisk(const tinygltf::Image& image, const std::string images_folder);
    MipmapInfo MergeOcclusionWithMetallicRoughness(MipmapInfo& occlusion_map, MipmapInfo& metallicRoughness_map);

    void LoadImageToGPU();

    void InitQuadIndexBuffer();
    void InitQuadPositionBuffer();
    void InitQuadTexcoordBuffer();
    void InitComputeSampler();
    void InitRenderpassSampler();

    void InitGPUimageAndView(uint8_t* in_image_raw, size_t image_size);

    std::unique_ptr<uint8_t[]> CopyToLocalBuffer(uint8_t* in_buffer, size_t buffer_size, bool shouldRGBtoRGBA);

    std::unique_ptr<uint8_t[]> CopyDeviceImageToLocalBuffer(Anvil::Buffer* in_buffer, size_t image_size, bool shouldRGBAtoRGB);

private: //data
    std::string imagesFolder;
    ImageAbout imageAbout;

    std::string shaderSetName;

    bool isImageLoaded = false;

    Anvil::BufferUniquePtr quadIndexBuffer_uptr;
    Anvil::BufferUniquePtr quadPositionBuffer_uptr;
    Anvil::BufferUniquePtr quadTexcoordsBuffer_uptr;

    std::unique_ptr<uint8_t[]> localDefaultImage_buffer;

    uint32_t original_width;
    uint32_t original_height;
    uint32_t defaultImageCompCount;
    uint32_t alignedImageCompCount;
    Anvil::Format vulkanDefaultFormat;
    Anvil::Format vulkanAlignedFormat;
    size_t alignedImageSize;

    Anvil::SamplerUniquePtr imageComputeSampler_uptr;
    Anvil::SamplerUniquePtr imageRenderpassSampler_uptr;

    Anvil::ImageUniquePtr alignedDefault_8bitPerChannel_image_uptr;
    Anvil::ImageViewUniquePtr alignedDefault_8bitPerChannel_imageView_uptr;

    std::string _16bitTo8bit_shadername;
    std::string baseColor_shadername;
    std::string occlusionMetallicRoughness_shadername;
    std::string normal_shadername;
    std::string emissive_shadername;

    PipelinesFactory* pipelinesFactory_ptr;
    ShadersSetsFamiliesCache* shadersSetsFamiliesCache_ptr;
    ImagesAboutOfTextures* imagesAboutOfTextures_ptr;

    Anvil::PrimaryCommandBuffer* const cmd_buffer_ptr;
    Anvil::BaseDevice* const device_ptr;
};