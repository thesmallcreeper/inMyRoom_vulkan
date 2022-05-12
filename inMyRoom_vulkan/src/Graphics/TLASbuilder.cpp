#include "Graphics/TLASbuilder.h"
#include "Graphics/Graphics.h"

#include "common/defines.h"

TLASbuilder::TLASbuilder(vk::Device in_device,
                         vma::Allocator in_vma_allocator,
                         uint32_t in_queue_family_index,
                         size_t max_instances)
    :device(in_device),
     vma_allocator(in_vma_allocator),
     queue_family_index(in_queue_family_index),
     maxInstances(max_instances)
{
    InitBuffers();
    InitTLASes();
    InitDescriptos();
}

TLASbuilder::~TLASbuilder()
{
    device.destroy(descriptorPool);
    device.destroy(TLASdescriptorSetLayout);

    device.destroy(TLASesHandles[0]);
    device.destroy(TLASesHandles[1]);
    vma_allocator.destroyBuffer(TLASesBuffer, TLASesAllocation);
    vma_allocator.destroyBuffer(TLASbuildScratchBuffer, TLASbuildScratchAllocation);
    vma_allocator.destroyBuffer(TLASesInstancesBuffer, TLASesInstancesAllocation);
}

void TLASbuilder::InitBuffers()
{
    {   // TLASesInstancesBuffer
        TLASesInstancesPartSize = sizeof(vk::AccelerationStructureInstanceKHR) * maxInstances;

        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = TLASesInstancesPartSize * 3;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eAccelerationStructureBuildInputReadOnlyKHR
                                   | vk::BufferUsageFlagBits::eTransferDst
                                   | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eCpuToGpu;
        buffer_allocation_create_info.flags = vma::AllocationCreateFlagBits::eMapped;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info,
                                                              buffer_allocation_create_info,
                                                              TLASesInstancesAllocInfo);

        assert(createBuffer_result.result == vk::Result::eSuccess);
        TLASesInstancesBuffer = createBuffer_result.value.first;
        TLASesInstancesAllocation = createBuffer_result.value.second;
    }
}

void TLASbuilder::InitTLASes()
{
    // Get required sizes
    vk::AccelerationStructureBuildSizesInfoKHR build_size_info;
    {
        vk::AccelerationStructureGeometryKHR instances;
        instances.geometryType = vk::GeometryTypeKHR::eInstances;
        instances.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;

        vk::AccelerationStructureBuildGeometryInfoKHR geometry_info;
        geometry_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        geometry_info.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
        geometry_info.geometryCount = 1;
        geometry_info.pGeometries = &instances;

        build_size_info = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                                       geometry_info,
                                                                       maxInstances);
    }
    TLASesHalfSize = build_size_info.accelerationStructureSize;
    // TODO: Vendor specific
    TLASesHalfSize += (TLASesHalfSize % 256 != 0) ? 256 - TLASesHalfSize % 256 : 0;

    // Create buffer for acceleration structures
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = TLASesHalfSize * 2;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo allocation_create_info;
        allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto create_buffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
        assert(create_buffer_result.result == vk::Result::eSuccess);
        TLASesBuffer = create_buffer_result.value.first;
        TLASesAllocation = create_buffer_result.value.second;
    }

    // Create TLASes
    for(size_t i = 0; i != 2; ++i) {
        vk::AccelerationStructureCreateInfoKHR TLAS_create_info;
        TLAS_create_info.buffer = TLASesBuffer;
        TLAS_create_info.size = build_size_info.accelerationStructureSize;
        TLAS_create_info.offset = i * TLASesHalfSize;
        TLAS_create_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
        auto TLAS_create_result = device.createAccelerationStructureKHR(TLAS_create_info);
        assert(TLAS_create_result.result == vk::Result::eSuccess);
        TLASesHandles[i] = TLAS_create_result.value;
        TLASesDeviceAddresses[i] = device.getAccelerationStructureAddressKHR({TLASesHandles[i]});
    }

    // Create scratch build buffer
    {
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = build_size_info.buildScratchSize;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo allocation_create_info;
        allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        TLASbuildScratchBuffer = createBuffer_result.value.first;
        TLASbuildScratchAllocation = createBuffer_result.value.second;
    }
}

void TLASbuilder::InitDescriptos()
{
    {   // Create descriptor pool
        std::vector<vk::DescriptorPoolSize> descriptor_pool_sizes;
        descriptor_pool_sizes.emplace_back(vk::DescriptorType::eAccelerationStructureKHR, 2);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({}, 2,
                                                                 descriptor_pool_sizes);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;
    }

    {   // Create layout
        std::vector<vk::DescriptorSetLayoutBinding> bindings;
        {   // TLAS
            vk::DescriptorSetLayoutBinding TLAS_binding;
            TLAS_binding.binding = 0;
            TLAS_binding.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
            TLAS_binding.descriptorCount = 1;
            TLAS_binding.stageFlags = vk::ShaderStageFlagBits::eFragment;

            bindings.emplace_back(TLAS_binding);
        }

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({}, bindings);
        TLASdescriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;
    }

    {   // Allocate sets
        std::vector<vk::DescriptorSetLayout> layouts;
        layouts.emplace_back(TLASdescriptorSetLayout);
        layouts.emplace_back(TLASdescriptorSetLayout);

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, layouts);
        std::vector<vk::DescriptorSet> descriptor_sets = device.allocateDescriptorSets(descriptor_set_allocate_info).value;
        TLASdescriptorSets[0] = descriptor_sets[0];
        TLASdescriptorSets[1] = descriptor_sets[1];
    }

    {   // Write descriptors of renderer sets
        std::vector<vk::WriteDescriptorSet> writes_descriptor_set;
        std::vector<std::unique_ptr<vk::DescriptorBufferInfo>> descriptor_buffer_infos_uptrs;
        std::vector<std::unique_ptr<vk::DescriptorImageInfo>> descriptor_image_infos_uptrs;
        std::vector<std::unique_ptr<vk::WriteDescriptorSetAccelerationStructureKHR>> acceleration_structures_pnext_uptrs;
        std::vector<vk::AccelerationStructureKHR> TLAS_handles = {GetTLAShandle(0), GetTLAShandle(1)};

        for (size_t i = 0; i != 2; ++i) {
                auto acceleration_structures_pnext_uptr = std::make_unique<vk::WriteDescriptorSetAccelerationStructureKHR>();
                acceleration_structures_pnext_uptr->accelerationStructureCount = 1;
                acceleration_structures_pnext_uptr->pAccelerationStructures = &TLAS_handles[i];

                vk::WriteDescriptorSet write_descriptor_set;
                write_descriptor_set.dstSet = TLASdescriptorSets[i];
                write_descriptor_set.dstBinding = 0;
                write_descriptor_set.dstArrayElement = 0;
                write_descriptor_set.descriptorCount = 1;
                write_descriptor_set.descriptorType = vk::DescriptorType::eAccelerationStructureKHR;
                write_descriptor_set.pNext = acceleration_structures_pnext_uptr.get();

                acceleration_structures_pnext_uptrs.emplace_back(std::move(acceleration_structures_pnext_uptr));
                writes_descriptor_set.emplace_back(write_descriptor_set);
        }

        device.updateDescriptorSets(writes_descriptor_set, {});
    }
}


std::vector<vk::AccelerationStructureInstanceKHR> TLASbuilder::CreateTLASinstances(const std::vector<DrawInfo>& draw_infos,
                                                                                   const std::vector<ModelMatrices>& matrices,
                                                                                   uint32_t device_buffer_index,
                                                                                   Graphics *graphics_ptr)
{
    std::vector<vk::AccelerationStructureInstanceKHR> return_vector;
    for (const auto& this_draw_info : draw_infos) {
        const MeshInfo& mesh_info = graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw_info.meshIndex);

        vk::AccelerationStructureInstanceKHR instance;
        const glm::mat4& matrix = matrices[this_draw_info.matricesOffset].positionMatrix;
        instance.transform = { matrix[0][0], matrix[1][0], matrix[2][0], matrix[3][0],
                               matrix[0][1], matrix[1][1], matrix[2][1], matrix[3][1],
                               matrix[0][2], matrix[1][2], matrix[2][2], matrix[3][2] };
        instance.instanceCustomIndex = this_draw_info.primitivesInstanceOffset;
        instance.instanceShaderBindingTableRecordOffset = 0;
        instance.flags = mesh_info.meshBLAS.disableFaceCulling ? uint8_t(vk::GeometryInstanceFlagBitsKHR::eTriangleFacingCullDisable) : 0;

        bool has_dynamic_shape = (this_draw_info.dynamicMeshIndex != -1)
                                 && graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex).hasDynamicShape;
        if (has_dynamic_shape) {
            const DynamicMeshInfo& dynamic_mesh_info = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex);
            instance.accelerationStructureReference = dynamic_mesh_info.BLASesDeviceAddresses[device_buffer_index];
        } else {
            instance.accelerationStructureReference = mesh_info.meshBLAS.deviceAddress;
        }

        bool is_light = this_draw_info.isLightSource;
        if (is_light) {
            instance.mask = LIGHT_MASK;
        } else {
            instance.mask = MESH_MASK;
        }
        return_vector.emplace_back(instance);
    }

    return return_vector;
}

void TLASbuilder::RecordTLASupdate(vk::CommandBuffer command_buffer,
                                   uint32_t host_buffer_index,
                                   uint32_t device_buffer_index,
                                   uint32_t TLAS_instances_count)
{
    vk::AccelerationStructureGeometryKHR geometry_instance;
    geometry_instance.geometryType = vk::GeometryTypeKHR::eInstances;
    geometry_instance.geometry.instances.sType = vk::StructureType::eAccelerationStructureGeometryInstancesDataKHR;
    geometry_instance.geometry.instances.arrayOfPointers = VK_FALSE;
    geometry_instance.geometry.instances.data = device.getBufferAddress(TLASesInstancesBuffer) + host_buffer_index * TLASesInstancesPartSize;

    vk::AccelerationStructureBuildGeometryInfoKHR geometry_info;
    geometry_info.type = vk::AccelerationStructureTypeKHR::eTopLevel;
    geometry_info.flags = vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
    geometry_info.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
    geometry_info.dstAccelerationStructure = TLASesHandles[device_buffer_index];
    geometry_info.geometryCount = 1;
    geometry_info.pGeometries = &geometry_instance;
    geometry_info.scratchData = device.getBufferAddress(TLASbuildScratchBuffer);

    vk::AccelerationStructureBuildRangeInfoKHR build_range = {};
    build_range.primitiveCount = TLAS_instances_count;

    vk::AccelerationStructureBuildRangeInfoKHR *indirection = &build_range;
    command_buffer.buildAccelerationStructuresKHR(1, &geometry_info, &indirection);
}

void TLASbuilder::TransferTLASrange(vk::CommandBuffer command_buffer,
                                    uint32_t device_buffer_index,
                                    uint32_t dst_family_index)
{
    if (queue_family_index == dst_family_index)
        return;

    vk::BufferMemoryBarrier this_memory_barrier = GetGenericTLASrangesBarrier(device_buffer_index);
    this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
    this_memory_barrier.dstQueueFamilyIndex = dst_family_index;

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                   vk::PipelineStageFlagBits::eBottomOfPipe,
                                   vk::DependencyFlagBits::eByRegion,
                                   {},
                                   this_memory_barrier,
                                   {});
}

void TLASbuilder::ObtainTLASranges(vk::CommandBuffer command_buffer,
                                   uint32_t device_buffer_index,
                                   uint32_t source_family_index)
{
    if (queue_family_index == source_family_index)
        return;

    vk::BufferMemoryBarrier this_memory_barrier = GetGenericTLASrangesBarrier(device_buffer_index);
    this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eAccelerationStructureWriteKHR;
    this_memory_barrier.srcQueueFamilyIndex = source_family_index;

    command_buffer.pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe,
                                   vk::PipelineStageFlagBits::eAccelerationStructureBuildKHR,
                                   vk::DependencyFlagBits::eByRegion,
                                   {},
                                   this_memory_barrier,
                                   {});
}

void TLASbuilder::WriteHostInstanceBuffer(const std::vector<vk::AccelerationStructureInstanceKHR>& TLAS_instances,
                                          uint32_t host_buffer_index) const
{
    host_buffer_index = host_buffer_index % 3;

    {
        memcpy((std::byte *) (TLASesInstancesAllocInfo.pMappedData) + host_buffer_index * TLASesInstancesPartSize,
               TLAS_instances.data(),
               TLAS_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));

        vma_allocator.flushAllocation(TLASesInstancesAllocation,
                                      host_buffer_index * TLASesInstancesPartSize,
                                      TLAS_instances.size() * sizeof(vk::AccelerationStructureInstanceKHR));
    }
}

vk::BufferMemoryBarrier TLASbuilder::GetGenericTLASrangesBarrier(uint32_t buffer_index) const
{
    buffer_index = buffer_index % 2;

    vk::BufferMemoryBarrier this_memory_barrier;
    this_memory_barrier.srcAccessMask = vk::AccessFlagBits::eNoneKHR;
    this_memory_barrier.dstAccessMask = vk::AccessFlagBits::eNoneKHR;
    this_memory_barrier.srcQueueFamilyIndex = queue_family_index;
    this_memory_barrier.dstQueueFamilyIndex = queue_family_index;
    this_memory_barrier.buffer = TLASesBuffer;
    this_memory_barrier.offset = buffer_index * TLASesHalfSize;
    this_memory_barrier.size = TLASesHalfSize;

    return this_memory_barrier;
}
