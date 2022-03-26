#pragma once

#include "vulkan/vulkan.hpp"
#include "vk_mem_alloc.hpp"
#include "ECS/ECStypes.h"
#include "common/structs/ModelMatrices.h"

// TODO change interface to match others

class TLASbuilder
{
public:
    TLASbuilder(vk::Device device,
                vma::Allocator vma_allocator,
                uint32_t queue_family_index,
                size_t max_instances);

    ~TLASbuilder();

    vk::DescriptorSet GetDescriptorSet(uint32_t index) {return TLASdescriptorSets[index];}
    vk::DescriptorSetLayout GetDescriptorSetLayout() {return TLASdescriptorSetLayout;}

    vk::BufferMemoryBarrier GetGenericTLASrangesBarrier(uint32_t buffer_index) const;

    void ObtainTLASranges(vk::CommandBuffer command_buffer,
                          uint32_t device_buffer_index,
                          uint32_t source_family_index);
    void RecordTLASupdate(vk::CommandBuffer command_buffer,
                          uint32_t host_buffer_index,
                          uint32_t device_buffer_index,
                          uint32_t TLAS_instances_count);
    void TransferTLASrange(vk::CommandBuffer command_buffer,
                           uint32_t device_buffer_index,
                           uint32_t dst_family_index);
    void WriteHostInstanceBuffer(const std::vector<vk::AccelerationStructureInstanceKHR>& instances_buffer,
                                 uint32_t host_buffer_index) const;

    static std::vector<vk::AccelerationStructureInstanceKHR> CreateTLASinstances(const std::vector<DrawInfo>& draw_infos,
                                                                                 const std::vector<ModelMatrices>& matrices,
                                                                                 uint32_t device_buffer_index,
                                                                                 class Graphics *graphics_ptr);

private:
    void InitBuffers();
    void InitTLASes();
    void InitDescriptos();

    vk::AccelerationStructureKHR GetTLAShandle(uint32_t index) {return TLASesHandles[index];}

private:
    vk::Buffer              TLASesInstancesBuffer;
    vma::Allocation         TLASesInstancesAllocation;
    vma::AllocationInfo     TLASesInstancesAllocInfo;
    size_t                  TLASesInstancesPartSize;

    vk::Buffer              TLASesBuffer;
    vma::Allocation         TLASesAllocation;
    vk::AccelerationStructureKHR TLASesHandles[2];
    uint64_t                TLASesDeviceAddresses[2];
    size_t                  TLASesHalfSize;

    vk::Buffer              TLASbuildScratchBuffer;
    vma::Allocation         TLASbuildScratchAllocation;

    vk::DescriptorPool      descriptorPool;
    vk::DescriptorSet       TLASdescriptorSets[2];
    vk::DescriptorSetLayout TLASdescriptorSetLayout;

    vk::Device              device;
    vma::Allocator          vma_allocator;
    const uint32_t          queue_family_index;
    const size_t            maxInstances;
};