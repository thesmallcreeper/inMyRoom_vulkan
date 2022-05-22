#pragma once

#include <unordered_map>

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

#include "Graphics/PipelinesFactory.h"

#include "NRD.h"
#include "NRDDescs.h"
#include "NRDSettings.h"

class NRDintegration
{
    class NRDstaticSamplers
    {
    public:
        NRDstaticSamplers(vk::Device vk_device);
        ~NRDstaticSamplers();

        vk::Sampler* GetVKsampler(nrd::Sampler nrd_sampler);
    private:
        std::unordered_map<nrd::Sampler, vk::Sampler> nrdSamplerToVKsampler_umap;

        vk::Device device;
    };

    class NRDtextureWrapper
    {
    public:
        NRDtextureWrapper() = default;
        NRDtextureWrapper(vk::Device vk_device, vma::Allocator vma_allocator, nrd::Format nrd_format, uint32_t width, uint32_t height, uint32_t mipLevels);
        NRDtextureWrapper(vk::Device vk_device, vk::Image in_image, vk::ImageCreateInfo in_image_info);
        ~NRDtextureWrapper();
        NRDtextureWrapper(const NRDtextureWrapper&) = delete;
        NRDtextureWrapper& operator=(const NRDtextureWrapper& other) = delete;

        NRDtextureWrapper(NRDtextureWrapper&& other) noexcept;
        NRDtextureWrapper& operator=(NRDtextureWrapper&& other) noexcept;

        std::vector<vk::ImageMemoryBarrier> SetLayout(vk::ImageLayout dst_layout, uint32_t mip_offset = 0, uint32_t mip_count = VK_REMAINING_MIP_LEVELS);
        vk::DescriptorImageInfo GetDescriptorImageInfo(uint32_t mip_offset = 0, uint32_t mip_count = 0);

        bool IsValidTexture() const {return isValidTexture;}

    private:
        void Deinit();

        vk::ImageView GetImageView(uint32_t mip_offset, uint32_t mip_count);
        vk::Format NRDtoVKformat(nrd::Format nrd_format);

        vk::Image image;
        vk::ImageCreateInfo imageInfo;
        std::unordered_map<uint64_t, vk::ImageView> mipOffsetCountPairToImageView_umap;

        std::vector<vk::ImageLayout> mipLayouts;

        bool wrapperImageOwner = false;
        vma::Allocation imageAllocation;
        vk::Device device = {};
        vma::Allocator vma_allocator = {};

        bool isValidTexture = false;
    };


public:
    NRDintegration(vk::Device vk_device,
                   vma::Allocator vma_allocator,
                   PipelinesFactory* pipelinesFactory_ptr,
                   const nrd::DenoiserCreationDesc& denoiserCreationDesc);
    ~NRDintegration();
    NRDintegration(const NRDintegration&) = delete;

    void BindTexture(nrd::ResourceType nrd_resource_type, vk::Image image, vk::ImageCreateInfo image_info,
                     vk::ImageLayout initial_layout, vk::ImageLayout final_layout);
    void SetMethodSettings(nrd::Method method, const void* methodSettings);
    void PrepareNewFrame(size_t frame_index, const nrd::CommonSettings& commonSettings);
    void Denoise(vk::CommandBuffer command_buffer,
                 vk::PipelineStageFlagBits src_stage = vk::PipelineStageFlagBits::eAllCommands,
                 vk::PipelineStageFlagBits dst_stage = vk::PipelineStageFlagBits::eAllCommands);
private:
    void Initialize(const nrd::DenoiserCreationDesc& denoiserCreationDesc);
    void CreateLayoutsAndPipelines();
    void CreateDescriptorSets();
    void CreateTexturesBuffers();

private:
    const nrd::DispatchDesc* NRDdispatches_ptr = nullptr;
    uint32_t NRDdispatches_count = 0;
    std::vector<std::vector<vk::ImageMemoryBarrier>> imageBarriersPerDispatch;

    std::array<NRDtextureWrapper, (uint32_t)nrd::ResourceType::MAX_NUM - 2> userTexturePool;
    std::array<vk::ImageLayout, (uint32_t)nrd::ResourceType::MAX_NUM - 2> userTexturePoolInitialLayout;
    std::array<vk::ImageLayout, (uint32_t)nrd::ResourceType::MAX_NUM - 2> userTexturePoolFinalLayout;

    std::vector<NRDtextureWrapper> privateTexturePool;
    vk::Buffer constantBuffer;
    vma::Allocation constantBufferAllocation;
    vma::AllocationInfo constantBufferAllocationInfo;
    size_t constantBufferPerSetSize;
    size_t constantBufferRangeSize;

    vk::DescriptorPool descriptorPool;
    std::array<std::vector<vk::DescriptorSet>, 3> descriptorSets;

    std::vector<vk::DescriptorSetLayout> descriptorSetLayouts;
    std::vector<vk::PipelineLayout> pipelineLayouts;
    std::vector<vk::ShaderModule> modules;
    std::vector<vk::Pipeline> pipelines;

    NRDstaticSamplers staticSamplers;

    nrd::Denoiser* NRDdenoiser_ptr = nullptr;

    size_t frameIndex = 0;

    PipelinesFactory * const pipelinesFactory_ptr;

    vk::Device device;
    vma::Allocator vma_allocator;
};