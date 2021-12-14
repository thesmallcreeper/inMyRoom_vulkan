#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"

class StagingBuffer
{
public:
    StagingBuffer(vk::Device device,
                  vma::Allocator allocator,
                  size_t size_byte);

    ~StagingBuffer();

    std::byte* GetDstPtr();
    vk::Buffer GetBuffer() {return stagingBuffer;};

    vk::CommandBuffer BeginCommandRecord(std::pair<vk::Queue, uint32_t> queue);
    void EndAndSubmitCommands();

private:
    vk::Buffer stagingBuffer;
    vma::Allocation stagingAllocation;
    vma::AllocationInfo stagingAllocInfo;
    size_t bufferSize = -1;

    vk::Device device;
    std::pair<vk::Queue, uint32_t> queue;
    vma::Allocator vma_allocator;

    vk::CommandPool commandPool;
    vk::CommandBuffer commandBuffer;

    bool recordingCommandBuffer = false;
    bool submitted = false;
};
