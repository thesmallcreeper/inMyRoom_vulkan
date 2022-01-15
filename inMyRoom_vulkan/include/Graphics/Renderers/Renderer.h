#pragma once
#include "Graphics/RendererBase.h"

#include "Geometry/FrustumCulling.h"

class Renderer
    : public RendererBase
{

    struct PrimitiveInstanceParameters {
        alignas(4) uint32_t indicesOffset = 0;      // uint32_t
        alignas(4) uint32_t positionOffset = 0;     // glm::vec4
        alignas(4) uint32_t normalOffset = 0;       // glm::vec4
        alignas(4) uint32_t tangentOffset = 0;      // glm::vec4
        alignas(4) uint32_t texcoordsOffset = 0;    // glm::vec2
        alignas(4) uint32_t colorOffset = 0;        // glm::vec4

        alignas(2) uint16_t matricesOffset = -1;
        alignas(2) uint16_t material = -1;
        alignas(2) uint16_t positionDescriptorIndex = 0;
        alignas(2) uint16_t normalDescriptorIndex = 0;
        alignas(2) uint16_t tangentDescriptorIndex = 0;
        alignas(2) uint16_t texcoordsDescriptorIndex = 0;
        alignas(2) uint16_t colorDescriptorIndex = 0;

        alignas(1) uint8_t indicesSetMultiplier = 3;
        alignas(1) uint8_t texcoordsStepMultiplier = 0;
        alignas(1) uint8_t colorStepMultiplier = 0;
    };
public:
    Renderer(class Graphics* in_graphics_ptr,
             vk::Device in_device,
             vma::Allocator in_vma_allocator);
    ~Renderer() override;

    void DrawFrame(ViewportFrustum viewport,
                   std::vector<glm::mat4>&& matrices,
                   std::vector<DrawInfo>&& draw_infos) override;

private:
    void InitBuffers();
    void InitDescriptors();
    void InitImages();
    void InitFramebuffers();
    void InitRenderpasses();
    void InitSemaphoresAndFences();
    void InitCommandBuffers();
    void InitPrimitivesSet();
    void InitFullscreenPipeline();

    void RecordCommandBuffer(vk::CommandBuffer command_buffer,
                             uint32_t buffer_index,
                             uint32_t swapchain_index,
                             const std::vector<DrawInfo>& draw_infos,
                             const FrustumCulling& frustum_culling);

    std::vector<PrimitiveInstanceParameters> GetPrimitivesInstanceParameters(std::vector<DrawInfo>& draw_infos) const;

private:
    std::pair<vk::Queue, uint32_t> graphicsQueue;
    size_t                  frameCount = 0;

    vk::Buffer              primitivesInstanceBuffer;
    vma::Allocation         primitivesInstanceAllocation;
    vma::AllocationInfo     primitivesInstanceAllocInfo;
    size_t                  primitivesInstanceBufferHalfsize;

    vk::Buffer              fullscreenBuffer;
    vma::Allocation         fullscreenAllocation;
    vma::AllocationInfo     fullscreenAllocInfo;
    size_t                  fullscreenBufferHalfsize;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       primitivesInstanceDescriptorSets[2];
    vk::DescriptorSetLayout primitivesInstanceDescriptorSetLayout;

    vk::Image               depthImage;
    vma::Allocation         depthAllocation;
    vk::ImageCreateInfo     depthImageCreateInfo;
    vk::ImageView           depthImageView;

    vk::Image               visibilityImage;
    vma::Allocation         visibilityAllocation;
    vk::ImageCreateInfo     visibilityImageCreateInfo;
    vk::ImageView           visibilityImageView;

    vk::RenderPass          renderpass;

    std::vector<vk::Framebuffer> framebuffers;

    vk::Semaphore           readyForPresentSemaphores[3];
    vk::Semaphore           presentImageAvailableSemaphores[3];
    vk::Semaphore           hostWriteFinishTimelineSemaphore;
    vk::Semaphore           submitFinishTimelineSemaphore;

    vk::CommandPool         commandPool;
    vk::CommandBuffer       commandBuffers[3];

    std::vector<vk::Pipeline>       primitivesPipelines;
    std::vector<vk::PipelineLayout> primitivesPipelineLayouts;

    vk::Pipeline            fullscreenPipeline;
    vk::PipelineLayout      fullscreenPipelineLayout;
};