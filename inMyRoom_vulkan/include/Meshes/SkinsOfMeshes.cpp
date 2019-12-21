#include "SkinsOfMeshes.h"

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"

SkinsOfMeshes::SkinsOfMeshes(Anvil::BaseDevice* const in_device_ptr, size_t in_swapchain_size, size_t in_nodes_buffer_size)
    :swapchainSize(in_swapchain_size),
     nodesMatrixesBufferSize(in_nodes_buffer_size),
     device_ptr(in_device_ptr)
{
    Anvil::MemoryAllocatorUniquePtr   allocator_ptr;

    const Anvil::MemoryFeatureFlags   required_feature_flags = Anvil::MemoryFeatureFlagBits::NONE;

    allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    nodesMatrixesBuffers_uptrs.resize(swapchainSize);

    for (size_t i = 0; i < swapchainSize; i++)
    {
        {
            auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                            nodesMatrixesBufferSize,
                                                                            Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                            Anvil::SharingMode::EXCLUSIVE,
                                                                            Anvil::BufferCreateFlagBits::NONE,
                                                                            Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT);

            nodesMatrixesBuffers_uptrs[i] = Anvil::Buffer::create(std::move(create_info_ptr));


            nodesMatrixesBuffers_uptrs[i]->set_name("Node's matrixes buffer " + std::to_string(i));
        }   
    

        allocator_ptr->add_buffer(nodesMatrixesBuffers_uptrs[i].get(),
                                  required_feature_flags);
    }
}

SkinsOfMeshes::~SkinsOfMeshes()
{
    nodesMatrixesBuffers_uptrs.clear();
    inverseBindMatrixesBuffer_uptr.reset();
}

void SkinsOfMeshes::AddSkinsOfModel(const tinygltf::Model& in_model)
{
    assert(!hasInverseBindMatrixesBeenFlashed);


}
