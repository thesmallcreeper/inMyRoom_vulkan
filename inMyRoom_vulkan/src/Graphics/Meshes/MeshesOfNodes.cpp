#include "Graphics/Meshes/MeshesOfNodes.h"

#include "Graphics/HelperUtils.h"

MeshesOfNodes::MeshesOfNodes(PrimitivesOfMeshes* in_primitivesOfMeshes_ptr,
                             vk::Device in_device,
                             vma::Allocator in_vma_allocator)
    :
    primitivesOfMeshes_ptr(in_primitivesOfMeshes_ptr),
    device(in_device),
    vma_allocator(in_vma_allocator)
{
}

MeshesOfNodes::~MeshesOfNodes()
{
    for(auto& this_mesh : meshes) {
        if (this_mesh.meshBLAS.hasBLAS) {
            device.destroy(this_mesh.meshBLAS.handle);
            vma_allocator.destroyBuffer(this_mesh.meshBLAS.buffer, this_mesh.meshBLAS.allocation);
        }
    }
}

void MeshesOfNodes::AddMeshesOfModel(const tinygltf::Model& in_model)
{
    modelToMeshIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), meshes.size());

    for (const tinygltf::Mesh& this_mesh : in_model.meshes)
    {
        MeshInfo this_mesh_info;
        size_t morphTargetsCount = 0;

        primitivesOfMeshes_ptr->StartRecordOBBtree();

        for (const tinygltf::Primitive& this_primitive : this_mesh.primitives) {
            size_t index = primitivesOfMeshes_ptr->AddPrimitive(in_model, this_primitive);

            this_mesh_info.primitivesIndex.emplace_back(index);
            this_mesh_info.isSkinned |= primitivesOfMeshes_ptr->IsPrimitiveSkinned(index);
            morphTargetsCount = std::max(primitivesOfMeshes_ptr->PrimitiveMorphTargetsCount(index), morphTargetsCount);
        }

        this_mesh_info.boundBoxTree = primitivesOfMeshes_ptr->GetOBBtreeAndReset();

        this_mesh_info.morphDefaultWeights = std::vector<float>(morphTargetsCount, 0.f);
        if (this_mesh.weights.size()) {
            assert(this_mesh_info.morphDefaultWeights.size() == this_mesh.weights.size());
            std::transform(this_mesh.weights.begin(), this_mesh.weights.end(), this_mesh_info.morphDefaultWeights.begin(),
                           [](double w) -> float { return float(w); });
        }

        meshes.emplace_back(std::move(this_mesh_info));
    }
}

size_t MeshesOfNodes::GetMeshIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToMeshIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToMeshIndexOffset_umap.end());

    return search->second;
}

void MeshesOfNodes::FlashDevice(std::pair<vk::Queue, uint32_t> queue)
{
    assert(not hasBeenFlashed);

    for(auto& this_mesh : meshes) {
        std::vector<vk::AccelerationStructureGeometryKHR> geometries;
        std::vector<vk::AccelerationStructureBuildRangeInfoKHR> geometries_ranges;
        std::vector<uint32_t> primitive_counts;

        // Get primitives geometries
        for(const size_t primitive_index : this_mesh.primitivesIndex) {
            const auto& this_geometry_tuple = primitivesOfMeshes_ptr->GetPrimitiveAccelerationStructureTriangle(primitive_index);

            if (std::get<0>(this_geometry_tuple)) {
                geometries.emplace_back(std::get<1>(this_geometry_tuple));
                geometries_ranges.emplace_back(std::get<2>(this_geometry_tuple));
                primitive_counts.emplace_back(std::get<2>(this_geometry_tuple).primitiveCount);
            }
        }

        if (geometries.size()) {
            this_mesh.meshBLAS.hasBLAS = true;

            vk::AccelerationStructureBuildGeometryInfoKHR geometry_info;
            geometry_info.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
            geometry_info.flags = this_mesh.IsSkinned() || this_mesh.HasMorphTargets() ?
                    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace | vk::BuildAccelerationStructureFlagBitsKHR::eAllowUpdate :
                    vk::BuildAccelerationStructureFlagBitsKHR::ePreferFastTrace;
            geometry_info.mode = vk::BuildAccelerationStructureModeKHR::eBuild;
            geometry_info.setGeometries(geometries);

            // Get size required
            vk::AccelerationStructureBuildSizesInfoKHR build_size_info;
            build_size_info = device.getAccelerationStructureBuildSizesKHR(vk::AccelerationStructureBuildTypeKHR::eDevice,
                                                                           geometry_info,
                                                                           primitive_counts);
            this_mesh.meshBLAS.bufferSize = build_size_info.accelerationStructureSize;
            this_mesh.meshBLAS.buildScratchBufferSize = build_size_info.buildScratchSize;
            this_mesh.meshBLAS.updateScratchBufferSize = build_size_info.updateScratchSize;

            // Create buffer for acceleration structure and creating it
            {
                vk::BufferCreateInfo buffer_create_info;
                buffer_create_info.size = build_size_info.accelerationStructureSize;
                buffer_create_info.usage = vk::BufferUsageFlagBits::eAccelerationStructureStorageKHR;
                buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

                vma::AllocationCreateInfo allocation_create_info;
                allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

                auto create_buffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
                assert(create_buffer_result.result == vk::Result::eSuccess);
                this_mesh.meshBLAS.buffer = create_buffer_result.value.first;
                this_mesh.meshBLAS.allocation = create_buffer_result.value.second;

                vk::AccelerationStructureCreateInfoKHR BLAS_create_info;
                BLAS_create_info.buffer = this_mesh.meshBLAS.buffer;
                BLAS_create_info.size = build_size_info.accelerationStructureSize;
                BLAS_create_info.type = vk::AccelerationStructureTypeKHR::eBottomLevel;
                auto BLAS_create_result = device.createAccelerationStructureKHR(BLAS_create_info);
                assert(BLAS_create_result.result == vk::Result::eSuccess);
                this_mesh.meshBLAS.handle = BLAS_create_result.value;
                this_mesh.meshBLAS.deviceAddress = device.getAccelerationStructureAddressKHR({this_mesh.meshBLAS.handle});
            }

            // Create scratch buffer
            vk::Buffer scratch_buffer;
            vma::Allocation scratch_allocation;
            {
                vk::BufferCreateInfo buffer_create_info;
                buffer_create_info.size = build_size_info.buildScratchSize;
                buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eShaderDeviceAddress;
                buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

                vma::AllocationCreateInfo allocation_create_info;
                allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

                auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, allocation_create_info);
                assert(createBuffer_result.result == vk::Result::eSuccess);
                scratch_buffer = createBuffer_result.value.first;
                scratch_allocation = createBuffer_result.value.second;
            }

            // Build BLAS
            geometry_info.scratchData.deviceAddress = device.getBufferAddress(scratch_buffer);
            geometry_info.dstAccelerationStructure  = this_mesh.meshBLAS.handle;

            OneShotCommandBuffer one_shot_command_buffer(device);
            vk::CommandBuffer command_buffer = one_shot_command_buffer.BeginCommandRecord(queue);

            vk::AccelerationStructureBuildRangeInfoKHR* indirection_ptr = geometries_ranges.data();
            command_buffer.buildAccelerationStructuresKHR(1, &geometry_info, &indirection_ptr);

            one_shot_command_buffer.EndAndSubmitCommands();

            // Delete scratch device
            vma_allocator.destroyBuffer(scratch_buffer, scratch_allocation);
        }
    }

    hasBeenFlashed = true;
}
