#include "SkinsOfMeshes.h"

#include <cassert>

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"

#include "glm/gtc/type_ptr.hpp"

#include "glTFenum.h"

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
    assert(!hasInitFlashed);

    for (const tinygltf::Skin& this_skin : in_model.skins)
    {
        SkinInfo this_skinInfo;
        this_skinInfo.inverseBindMatrixesFirstOffset = GetCountOfInverseBindMatrixes();
        this_skinInfo.inverseBindMatrixesSize = this_skin.joints.size();

        for (size_t index = 0; index < this_skin.joints.size(); index++)
        {
            this_skinInfo.glTFnodesJoints.emplace_back(this_skin.joints[index]);
            const glm::mat4 this_joint_inverseBindMatrix = GetAccessorMatrix(index,
                                                                             in_model,
                                                                             in_model.accessors[this_skin.inverseBindMatrices]);
            AddMatrixToInverseBindMatrixesBuffer(this_joint_inverseBindMatrix);
        }

        skins.emplace_back(this_skinInfo);
    }

    modelToSkinIndexOffset_umap.emplace(const_cast<tinygltf::Model*>(&in_model), skinsSoFar);

    skinsSoFar += in_model.skins.size();
}

void SkinsOfMeshes::FlashDevice()
{
    assert(!hasInitFlashed);

    inverseBindMatrixesBuffer_uptr = CreateDeviceBufferForLocalBuffer(localInverseBindMatrixesBuffer,
                                                                      Anvil::BufferUsageFlagBits::UNIFORM_BUFFER_BIT,
                                                                      "Inverse bind matrixes buffer");

    // Create DescriptorSetGroup
    {
        std::vector<Anvil::DescriptorSetCreateInfoUniquePtr> dsg_create_infos_uptrs;
        for (size_t i = 0; i < swapchainSize; i++)
        {
            // Create description set

            Anvil::DescriptorSetCreateInfoUniquePtr this_description_set_create_info_uptr = Anvil::DescriptorSetCreateInfo::create();

            // Add inverse binding matrices (bind: 0)

            this_description_set_create_info_uptr->add_binding(0 /*in_binding_index*/,
                                                               Anvil::DescriptorType::UNIFORM_BUFFER,
                                                               1 /*in_descriptor_array_size*/,
                                                               Anvil::ShaderStageFlagBits::VERTEX_BIT);

            // Add nodes matrixes buffer (bind: 1)

            this_description_set_create_info_uptr->add_binding(1, /*in_binding_index*/
                                                               Anvil::DescriptorType::UNIFORM_BUFFER,
                                                               1, /*in_descriptor_array_size*/
                                                               Anvil::ShaderStageFlagBits::VERTEX_BIT);

            dsg_create_infos_uptrs.emplace_back(std::move(this_description_set_create_info_uptr));

        }

        descriptorSetGroup_uptr = Anvil::DescriptorSetGroup::create(device_ptr,
                                                                    dsg_create_infos_uptrs,
                                                                    false);

        for (size_t i = 0; i < swapchainSize; i++)
        {
            // Bind inverse binding matrices (to bind: 0)
            {
                Anvil::DescriptorSet::UniformBufferBindingElement this_uniform_bind(inverseBindMatrixesBuffer_uptr.get(),
                                                                                    0, /*in_start_offset*/
                                                                                    inverseBindMatrixesBuffer_uptr->get_create_info_ptr()->get_size());

                descriptorSetGroup_uptr->set_binding_item(static_cast<uint32_t>(i), /*in_n_set*/
                                                          0, /*in_binding_index*/
                                                          this_uniform_bind);
            }

            // Bind nodes matrixes buffer (to bind: 1)
            {
                Anvil::DescriptorSet::UniformBufferBindingElement this_uniform_bind(nodesMatrixesBuffers_uptrs[i].get(),
                                                                                    0, /*in_start_offset*/
                                                                                    nodesMatrixesBuffers_uptrs[i]->get_create_info_ptr()->get_size());

                descriptorSetGroup_uptr->set_binding_item(static_cast<uint32_t>(i), /*in_n_set*/
                                                          1, /*in_binding_index*/
                                                          this_uniform_bind);
            }
        }
    }

    hasInitFlashed = true;
    assert(hasInitFlashed);
}

size_t SkinsOfMeshes::GetSkinIndexOffsetOfModel(const tinygltf::Model& in_model) const
{
    auto search = modelToSkinIndexOffset_umap.find(const_cast<tinygltf::Model*>(&in_model));

    assert(search != modelToSkinIndexOffset_umap.end());

    return search->second;
}

SkinInfo SkinsOfMeshes::GetSkin(size_t this_skin_index) const
{
    return skins[this_skin_index];
}

void SkinsOfMeshes::StartRecordingNodesMatrixes(size_t in_swapchain)
{
    assert(!isRecording);
    isRecording = true;

    assert(in_swapchain < swapchainSize);
    currectSwapchainBeingRecorded = in_swapchain;

    localCurrectSwapchainNodesMatrixesBuffer.clear();

    assert(isRecording);
}

void SkinsOfMeshes::AddNodeMatrix(glm::mat4 in_node_matrix)
{
    assert(isRecording);

    std::copy(reinterpret_cast<unsigned char*>(&in_node_matrix),
              reinterpret_cast<unsigned char*>(&in_node_matrix + sizeof(glm::mat4)),
              std::back_inserter(localCurrectSwapchainNodesMatrixesBuffer));

    assert(localCurrectSwapchainNodesMatrixesBuffer.size() <= nodesMatrixesBufferSize);
}

size_t SkinsOfMeshes::GetNodesRecordSize() const
{
    return localCurrectSwapchainNodesMatrixesBuffer.size() / sizeof(glm::mat4);
}

void SkinsOfMeshes::EndRecodingAndFlash(Anvil::Queue* in_opt_flash_queue)
{
    assert(isRecording);

    nodesMatrixesBuffers_uptrs[currectSwapchainBeingRecorded]->write(0,
                                                                     localCurrectSwapchainNodesMatrixesBuffer.size(),
                                                                     localCurrectSwapchainNodesMatrixesBuffer.data(),
                                                                     in_opt_flash_queue);

    isRecording = false;
    assert(!isRecording);
}

const Anvil::DescriptorSetCreateInfo* SkinsOfMeshes::GetDescriptorSetCreateInfoPtr()
{
    assert(hasInitFlashed);

    return descriptorSetGroup_uptr->get_descriptor_set_create_info(0);
}

Anvil::DescriptorSet* SkinsOfMeshes::GetDescriptorSetPtr(size_t in_swapchain)
{
    assert(hasInitFlashed);

    return descriptorSetGroup_uptr->get_descriptor_set(static_cast<uint32_t>(in_swapchain));
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

    std::array<float, 16>  matrix_data;
    if (isDouble)
    {
        double* double_raw_data_ptr = reinterpret_cast<double*>(raw_data.data());
        for (size_t index = 0; index < matrix_data.size(); index++)
            matrix_data[index] = static_cast<float>(double_raw_data_ptr[index]);
    }
    else
    {
        float* float_raw_data_ptr = reinterpret_cast<float*>(raw_data.data());
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

void SkinsOfMeshes::AddMatrixToInverseBindMatrixesBuffer(const glm::mat4 this_matrix)
{
    const unsigned char* this_matrix_raw_ptr = reinterpret_cast<const unsigned char*>(&this_matrix);

    std::copy(this_matrix_raw_ptr,
              this_matrix_raw_ptr + sizeof(glm::mat4),
              std::back_inserter(localInverseBindMatrixesBuffer));
}

size_t SkinsOfMeshes::GetCountOfInverseBindMatrixes() const
{
    return localInverseBindMatrixesBuffer.size() / sizeof(glm::mat4);
}

Anvil::BufferUniquePtr SkinsOfMeshes::CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
                                                                       Anvil::BufferUsageFlagBits in_bufferusageflag,
                                                                       std::string buffers_name) const
{
    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    in_localBuffer.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    in_bufferusageflag);

    Anvil::BufferUniquePtr buffer_uptr = Anvil::Buffer::create(std::move(create_info_ptr));

    buffer_uptr->set_name(buffers_name);

    auto allocator_uptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    allocator_uptr->add_buffer(buffer_uptr.get(),
                               Anvil::MemoryFeatureFlagBits::NONE);

    buffer_uptr->write(0,
                       in_localBuffer.size(),
                       in_localBuffer.data());

    return std::move(buffer_uptr);
}

