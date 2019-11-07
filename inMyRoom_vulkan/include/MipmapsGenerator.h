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

#include "misc/image_create_info.h"
#include "misc/image_view_create_info.h"
#include "misc/framebuffer_create_info.h"
#include "misc/sampler_create_info.h"
#include "misc/render_pass_create_info.h"
#include "misc/buffer_create_info.h"
#include "misc/memory_allocator.h"
#include "misc/fence_create_info.h"
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

#include "PipelinesFactory.h"
#include "ImagesAboutOfTextures.h"
#include "ShadersSetsFamiliesCache.h"

struct MipmapInfo
{
    Anvil::Format image_vulkan_format;
    size_t size;
    size_t pitch;
    size_t width;
    size_t height;
    unique_ptr<uint8_t[]> data_uptr;
};


class MipmapsGenerator
{
public:
    MipmapsGenerator(PipelinesFactory* pipelinesFactory_ptr,
                     ShadersSetsFamiliesCache* in_shadersSetsFamiliesCache_ptr,
                     ImagesAboutOfTextures* in_imagesAboutOfTextures_ptr,
                     std::string in_16bitTo8bit_shadername,
                     std::string baseColor_shadername,
                     std::string metallic_shadername,
                     std::string roughness_shadername,
                     std::string normal_shadername,
                     std::string occlusion_shadername,
                     std::string emissive_shadername,
                     Anvil::PrimaryCommandBuffer* const in_cmd_buffer_ptr,
                     Anvil::BaseDevice* const in_device_ptr);
    ~MipmapsGenerator();

    void Reset();
    void ResetAndCopyImage(const tinygltf::Image& in_image ,const std::string in_imagesFolder);

    MipmapInfo GetOriginal();
    MipmapInfo GetOriginalNullptr();
    MipmapInfo GetMipmap(size_t mipmap_level);
    size_t GetMipmaps_levels_over_4x4();

private: //functions

    void Init();

    std::string GetShaderSetName(ImageMap map) const;

    std::unique_ptr<uint8_t[]> CopyToLocalBuffer(uint8_t* in_buffer, size_t buffer_size, bool shouldRGBtoRGBA);

    std::unique_ptr<uint8_t[]> CopyDeviceImageToLocalBuffer(Anvil::Buffer* in_buffer, size_t image_size, bool shouldRGBAtoRGB);

private: //data
    std::string imagesFolder;

    bool isItInited = false;

    Anvil::BufferUniquePtr quadIndexBuffer_uptr;
    Anvil::BufferUniquePtr quadPositionBuffer_uptr;
    Anvil::BufferUniquePtr quadTexcoordsBuffer_uptr;

    std::unique_ptr<uint8_t[]> default_image_buffer;

    int32_t original_width;
    int32_t original_height;
    int32_t defaultImageCompCount;
    int32_t originalImageCompCount;
    Anvil::Format vulkanDefaultFormat;
    Anvil::Format vulkanOriginalFormat;

    ImageAbout image_about;

    Anvil::SamplerUniquePtr imageSampler_uptr;

    Anvil::ImageUniquePtr original_8bitPerChannel_image_uptr;
    Anvil::ImageViewUniquePtr original_8bitPerChannel_imageView_uptr;

    std::string _16bitTo8bit_shadername;
    std::string baseColor_shadername;
    std::string metallic_shadername;
    std::string roughness_shadername;
    std::string normal_shadername;
    std::string occlusion_shadername;
    std::string emissive_shadername;

    PipelinesFactory* pipelinesFactory_ptr;
    ShadersSetsFamiliesCache* shadersSetsFamiliesCache_ptr;
    ImagesAboutOfTextures* imagesAboutOfTextures_ptr;

    Anvil::PrimaryCommandBuffer* const cmd_buffer_ptr;
    Anvil::BaseDevice* const device_ptr;
};