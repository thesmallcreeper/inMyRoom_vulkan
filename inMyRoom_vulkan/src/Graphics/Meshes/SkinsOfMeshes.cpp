#include "Graphics/Meshes/SkinsOfMeshes.h"

#include <cassert>
#include <iterator>

#include "glm/gtc/type_ptr.hpp"
#include "glm/gtc/matrix_inverse.hpp"

#include "glTFenum.h"
#include "Graphics/HelperUtils.h"

#include "common/structs/ModelMatrices.h"

SkinsOfMeshes::SkinsOfMeshes(vk::Device in_device, vma::Allocator in_vma_allocator)
    :device(in_device),
     vma_allocator(in_vma_allocator)
{
    // Padding
    glm::mat4 identity(1.f);
    inverseBindMatrices.emplace_back(identity);
}


SkinsOfMeshes::~SkinsOfMeshes()
{
    device.destroy(descriptorSetLayout);
    device.destroy(descriptorPool);

    vma_allocator.destroyBuffer(inverseBindBuffer, inverseBindAllocation);
}

void SkinsOfMeshes::AddSkinsOfModel(const tinygltf::Model& in_model)
{
    assert(!hasBeenFlashed);

    modelToSkinIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), skinInfos.size());

    for (const tinygltf::Skin& this_skin : in_model.skins)
    {
        SkinInfo this_skinInfo;
        this_skinInfo.inverseBindMatricesFirstOffset = GetCountOfInverseBindMatrices();
        this_skinInfo.inverseBindMatricesSize = this_skin.joints.size();

        for (size_t index = 0; index < this_skin.joints.size(); index++)
        {
            this_skinInfo.glTFnodesJoints.emplace_back(this_skin.joints[index]);
            const glm::mat4 this_joint_inverseBindMatrix = GetAccessorMatrix(index,
                                                                             in_model,
                                                                             in_model.accessors[this_skin.inverseBindMatrices]);
            inverseBindMatrices.emplace_back(this_joint_inverseBindMatrix);
        }

        skinInfos.emplace_back(this_skinInfo);
    }
}

void SkinsOfMeshes::FlashDevice(std::pair<vk::Queue, uint32_t> queue)
{
    assert(!hasBeenFlashed);

    // Create and flash buffer
    size_t buffer_size_bytes = GetInverseBindMatricesBufferSize();
    {   // Create buffer
        vk::BufferCreateInfo buffer_create_info;
        buffer_create_info.size = buffer_size_bytes;
        buffer_create_info.usage = vk::BufferUsageFlagBits::eStorageBuffer | vk::BufferUsageFlagBits::eTransferDst;
        buffer_create_info.sharingMode = vk::SharingMode::eExclusive;

        vma::AllocationCreateInfo buffer_allocation_create_info;
        buffer_allocation_create_info.usage = vma::MemoryUsage::eGpuOnly;

        auto createBuffer_result = vma_allocator.createBuffer(buffer_create_info, buffer_allocation_create_info);
        assert(createBuffer_result.result == vk::Result::eSuccess);
        inverseBindBuffer = createBuffer_result.value.first;
        inverseBindAllocation = createBuffer_result.value.second;
    }
    {   // Transfer
        StagingBuffer staging_buffer(device, vma_allocator, buffer_size_bytes);
        std::byte *dst_ptr = staging_buffer.GetDstPtr();

        std::vector<ModelMatrices> inverseBind_modelMatrices;
        std::transform(inverseBindMatrices.begin(), inverseBindMatrices.end(), std::back_inserter(inverseBind_modelMatrices),
                       [](const glm::mat4& pos_matrix) {
                        glm::mat4 normal_matrix = glm::adjointTranspose(pos_matrix);
                        return ModelMatrices({pos_matrix, normal_matrix});
                       });

        memcpy(dst_ptr, inverseBind_modelMatrices.data(), buffer_size_bytes);

        vk::CommandBuffer command_buffer = staging_buffer.BeginCommandRecord(queue);
        vk::Buffer copy_buffer = staging_buffer.GetBuffer();

        vk::BufferCopy copy_region;
        copy_region.srcOffset = 0;
        copy_region.dstOffset = 0;
        copy_region.size = buffer_size_bytes;
        command_buffer.copyBuffer(copy_buffer, inverseBindBuffer, 1, &copy_region);

        staging_buffer.EndAndSubmitCommands();

        inverseBindMatricesCount = GetCountOfInverseBindMatrices();
        hasBeenFlashed = true;
        inverseBindMatrices.clear();
    }

    // Create and write descriptor set
    {   // Create descriptor set
        vk::DescriptorPoolSize descriptor_pool_size(vk::DescriptorType::eStorageBuffer, 1);
        vk::DescriptorPoolCreateInfo descriptor_pool_create_info({},1,
                                                                 1,&descriptor_pool_size);

        descriptorPool = device.createDescriptorPool(descriptor_pool_create_info).value;

        vk::DescriptorSetLayoutBinding descriptor_set_layout_binding;
        descriptor_set_layout_binding.binding = 0;
        descriptor_set_layout_binding.descriptorType = vk::DescriptorType::eStorageBuffer;
        descriptor_set_layout_binding.descriptorCount = 1;
        descriptor_set_layout_binding.stageFlags = vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eCompute;

        vk::DescriptorSetLayoutCreateInfo descriptor_set_layout_create_info({},1, &descriptor_set_layout_binding);
        descriptorSetLayout = device.createDescriptorSetLayout(descriptor_set_layout_create_info).value;

        vk::DescriptorSetAllocateInfo descriptor_set_allocate_info(descriptorPool, 1, &descriptorSetLayout);
        descriptorSet = device.allocateDescriptorSets(descriptor_set_allocate_info).value[0];
    }
    {   // Write descriptor set
        vk::DescriptorBufferInfo descriptor_buffer_info;
        descriptor_buffer_info.buffer = inverseBindBuffer;
        descriptor_buffer_info.offset = 0;
        descriptor_buffer_info.range = VK_WHOLE_SIZE;

        vk::WriteDescriptorSet write_descriptor_set;
        write_descriptor_set.dstSet = descriptorSet;
        write_descriptor_set.dstBinding = 0;
        write_descriptor_set.dstArrayElement = 0;
        write_descriptor_set.descriptorCount = 1;
        write_descriptor_set.descriptorType = vk::DescriptorType::eStorageBuffer;
        write_descriptor_set.pBufferInfo = &descriptor_buffer_info;

        device.updateDescriptorSets(1, &write_descriptor_set, 0, nullptr);
    }
}

size_t SkinsOfMeshes::GetSkinIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToSkinIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToSkinIndexOffset_umap.end());

    return search->second;
}

glm::mat4 SkinsOfMeshes::GetAccessorMatrix(const size_t index, const tinygltf::Model& in_model, const tinygltf::Accessor& in_accessor)
{
    bool isDouble = false;
    if (in_accessor.componentType == static_cast<int>(glTFcomponentType::type_double))
        isDouble = true;

    std::vector<unsigned char> raw_data;

    {
        size_t accessor_byte_offset = in_accessor.byteOffset;

        const tinygltf::BufferView& this_bufferView = in_model.bufferViews[in_accessor.bufferView];
        size_t bufferview_byte_offset = this_bufferView.byteOffset;

        const tinygltf::Buffer& this_buffer = in_model.buffers[this_bufferView.buffer];

        std::copy(&this_buffer.data[bufferview_byte_offset + accessor_byte_offset +  index      * sizeof(glm::mat4) * (isDouble ? 2 : 1)],
                  &this_buffer.data[bufferview_byte_offset + accessor_byte_offset + (index + 1) * sizeof(glm::mat4) * (isDouble ? 2 : 1)],
                  std::back_inserter(raw_data));
    }

    std::array<float, 16>  matrix_data = {};
    if (isDouble)
    {
        auto double_raw_data_ptr = reinterpret_cast<double*>(raw_data.data());
        for (size_t index = 0; index < matrix_data.size(); index++)
            matrix_data[index] = static_cast<float>(double_raw_data_ptr[index]);
    }
    else
    {
        auto float_raw_data_ptr = reinterpret_cast<float*>(raw_data.data());
        for (size_t index = 0; index < matrix_data.size(); index++)
            matrix_data[index] = static_cast<float>(float_raw_data_ptr[index]);
    }


    glm::mat4 return_matrix = glm::mat4(1.f, 0.f, 0.f, 0.f,
                                        0.f, -1.f, 0.f, 0.f,
                                        0.f, 0.f, -1.f, 0.f,
                                        0.f, 0.f, 0.f, 1.f)
                           * glm::make_mat4<float>(matrix_data.data())
                           * glm::mat4(1.f, 0.f, 0.f, 0.f,
                                       0.f, -1.f, 0.f, 0.f,
                                       0.f, 0.f, -1.f, 0.f,
                                       0.f, 0.f, 0.f, 1.f);

    return return_matrix;
}

size_t SkinsOfMeshes::GetCountOfInverseBindMatrices() const
{
    if( not hasBeenFlashed)
        return inverseBindMatrices.size();
    else
        return inverseBindMatricesCount;
}

size_t SkinsOfMeshes::GetInverseBindMatricesBufferSize() const
{
    return GetCountOfInverseBindMatrices()*sizeof(ModelMatrices);
}

