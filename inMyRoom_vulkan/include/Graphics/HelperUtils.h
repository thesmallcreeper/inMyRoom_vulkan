#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

class OneShotCommandBuffer
{
public:
    explicit OneShotCommandBuffer(vk::Device device);
    ~OneShotCommandBuffer();

    vk::CommandBuffer BeginCommandRecord(std::pair<vk::Queue, uint32_t> queue);
    void EndAndSubmitCommands();

private:
    vk::Device device;
    std::pair<vk::Queue, uint32_t> queue;

    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;

    bool recordingCommandBuffer = false;
    bool submitted = false;
};

class StagingBuffer
{
public:
    StagingBuffer(vk::Device device,
                  vma::Allocator allocator,
                  size_t size_byte);

    ~StagingBuffer();

    std::byte* GetDstPtr() const;
    vk::Buffer GetBuffer() const {return stagingBuffer;};

    vk::CommandBuffer BeginCommandRecord(std::pair<vk::Queue, uint32_t> queue);
    void EndAndSubmitCommands();

private:
    vk::Buffer stagingBuffer;
    vma::Allocation stagingAllocation;
    vma::AllocationInfo stagingAllocInfo;
    size_t bufferSize = -1;

    vk::Device device;
    vma::Allocator vma_allocator;

    OneShotCommandBuffer oneShotCommandBuffer;
};
