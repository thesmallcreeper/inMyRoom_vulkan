#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"
#include "ECS/ECStypes.h"
#include "common/structs/ModelMatrices.h"

class TLASinstance
{
public:
    TLASinstance(vk::Device device,
                 vma::Allocator vma_allocator,
                 uint32_t queue_family_index,
                 size_t max_instances);

    ~TLASinstance();

    vk::AccelerationStructureKHR GetTLAShandle(uint32_t index) {return TLASesHandles[index];}

    vk::BufferMemoryBarrier GetGenericTLASrangesBarrier(uint32_t buffer_index) const;

    void ObtainTLASranges(vk::CommandBuffer command_buffer,
                          uint32_t buffer_index,
                          uint32_t source_family_index);
    void RecordTLASupdate(vk::CommandBuffer command_buffer,
                          uint32_t buffer_index,
                          uint32_t TLAS_instances_count);
    void TransferTLASrange(vk::CommandBuffer command_buffer,
                           uint32_t buffer_index,
                           uint32_t dst_family_index);
    void WriteHostInstanceBuffer(const std::vector<vk::AccelerationStructureInstanceKHR>& instances_buffer,
                                 uint32_t buffer_index) const;

    static std::vector<vk::AccelerationStructureInstanceKHR> CreateTLASinstances(const std::vector<DrawInfo>& draw_infos,
                                                                                 const std::vector<ModelMatrices>& matrices,
                                                                                 uint32_t buffer_index,
                                                                                 class Graphics *graphics_ptr);

private:
    void InitBuffers();
    void InitTLASes();

private:
    vk::Buffer              TLASesInstancesBuffer;
    vma::Allocation         TLASesInstancesAllocation;
    vma::AllocationInfo     TLASesInstancesAllocInfo;
    size_t                  TLASesInstancesHalfSize;

    vk::Buffer              TLASesBuffer;
    vma::Allocation         TLASesAllocation;
    vk::AccelerationStructureKHR TLASesHandles[2];
    uint64_t                TLASesDeviceAddresses[2];
    size_t                  TLASesHalfSize;

    vk::Buffer              TLASbuildScratchBuffer;
    vma::Allocation         TLASbuildScratchAllocation;

    vk::Device              device;
    vma::Allocator          vma_allocator;
    const uint32_t          queue_family_index;
    const size_t            maxInstances;
};