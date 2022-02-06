#include "Graphics/HelperUtils.h"

OneShotCommandBuffer::OneShotCommandBuffer(vk::Device in_device)
    :
    device(in_device)
{
}

OneShotCommandBuffer::~OneShotCommandBuffer()
{
    assert(not recordingCommandBuffer || submitted);

    if (commandPool) {
        device.destroy(commandPool);
    }
}

vk::CommandBuffer OneShotCommandBuffer::BeginCommandRecord(std::pair<vk::Queue, uint32_t> in_queue)
{
    assert(not recordingCommandBuffer);
    assert(not submitted);
    queue = in_queue;

    {
        vk::CommandPoolCreateInfo command_pool_create_info;
        command_pool_create_info.flags = vk::CommandPoolCreateFlagBits::eTransient;
        command_pool_create_info.queueFamilyIndex = queue.second;

        commandPool = device.createCommandPool(command_pool_create_info).value;

        vk::CommandBufferAllocateInfo command_buffer_alloc_info;
        command_buffer_alloc_info.commandPool = commandPool;
        command_buffer_alloc_info.level = vk::CommandBufferLevel::ePrimary;
        command_buffer_alloc_info.commandBufferCount = 1;

        commandBuffer = device.allocateCommandBuffers(command_buffer_alloc_info).value[0];
    }

    // Record start
    commandBuffer.begin(vk::CommandBufferBeginInfo( vk::CommandBufferUsageFlagBits::eOneTimeSubmit ));
    recordingCommandBuffer = true;

    return commandBuffer;
}

void OneShotCommandBuffer::EndAndSubmitCommands()
{
    assert(recordingCommandBuffer);
    assert(not submitted);

    commandBuffer.end();
    recordingCommandBuffer = false;
    // Record has ended

    vk::SubmitInfo submit_info;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &commandBuffer;

    queue.first.submit(1, &submit_info, {});
    device.waitIdle();

    submitted = true;
}

StagingBuffer::StagingBuffer(vk::Device in_device, vma::Allocator in_allocator, size_t size_byte)
    :device(in_device),
     vma_allocator(in_allocator),
     bufferSize(size_byte),
     oneShotCommandBuffer(device)
{
    {
        vk::BufferCreateInfo staging_buffer_create_info;
        staging_buffer_create_info.size = bufferSize;
        staging_buffer_create_info.usage = vk::BufferUsageFlagBits::eTransferSrc;

        vma::AllocationCreateInfo staging_allocation_create_info;
        staging_allocation_create_info.usage = vma::MemoryUsage::eCpuOnly;
        staging_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;
        staging_allocation_create_info.requiredFlags = vk::MemoryPropertyFlagBits::eHostVisible | vk::MemoryPropertyFlagBits::eHostCoherent;

        auto createBuffer_result = vma_allocator.createBuffer(staging_buffer_create_info,
                                                              staging_allocation_create_info,
                                                              stagingAllocInfo);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        stagingBuffer = createBuffer_result.value.first;
        stagingAllocation = createBuffer_result.value.second;
    }
    assert(stagingAllocInfo.pMappedData != nullptr);
}

StagingBuffer::~StagingBuffer()
{
    if (stagingBuffer) {
        vma_allocator.destroyBuffer(stagingBuffer, stagingAllocation);
    }
}

std::byte *StagingBuffer::GetDstPtr() const
{
    return static_cast<std::byte*>(stagingAllocInfo.pMappedData);
}

vk::CommandBuffer StagingBuffer::BeginCommandRecord(std::pair<vk::Queue, uint32_t> in_queue)
{
    return oneShotCommandBuffer.BeginCommandRecord(in_queue);
}

void StagingBuffer::EndAndSubmitCommands()
{
    oneShotCommandBuffer.EndAndSubmitCommands();
}

