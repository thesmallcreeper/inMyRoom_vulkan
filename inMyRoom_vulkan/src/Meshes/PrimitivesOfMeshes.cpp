#include "Meshes/PrimitivesOfMeshes.h"

#include <limits>
#include <cassert>
#include <algorithm>
#include <iterator>

PrimitivesOfMeshes::PrimitivesOfMeshes(PipelinesFactory* in_pipelinesFactory_ptr,
                                       ShadersSetsFamiliesCache* in_shadersSetsFamiliesCache_ptr,
                                       MaterialsOfPrimitives* in_materialsOfPrimitives_ptr,
                                       Anvil::BaseDevice* const in_device_ptr)
    :
    pipelinesFactory_ptr(in_pipelinesFactory_ptr),
    shadersSetsFamiliesCache_ptr(in_shadersSetsFamiliesCache_ptr),
    materialsOfPrimitives_ptr(in_materialsOfPrimitives_ptr),
    device_ptr(in_device_ptr)
{
}

PrimitivesOfMeshes::~PrimitivesOfMeshes()
{
    indexBuffer_uptr.reset();
    positionBuffer_uptr.reset();
    normalBuffer_uptr.reset();
    tangentBuffer_uptr.reset();
    texcoord0Buffer_uptr.reset();
}

void PrimitivesOfMeshes::AddPrimitive(const tinygltf::Model& in_model,
                                      const tinygltf::Primitive& in_primitive)
{
    assert(hasBuffersBeenFlashed == false);

    PrimitiveGeneralInfo this_primitiveInitInfo;
    this_primitiveInitInfo.pipelineSpecs.drawMode = static_cast<glTFmode>(in_primitive.mode);
    this_primitiveInitInfo.materialIndex = materialsOfPrimitives_ptr->GetMaterialIndexOffsetOfModel(in_model) + in_primitive.material;

    {
        const tinygltf::Accessor& this_accessor = in_model.accessors[in_primitive.indices];

        this_primitiveInitInfo.indicesCount = static_cast<uint32_t>(this_accessor.count);

        if (this_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short))
            this_primitiveInitInfo.indexBufferType = Anvil::IndexType::UINT16;
        else if (this_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_int))
            this_primitiveInitInfo.indexBufferType = Anvil::IndexType::UINT32;
        else
            assert(0);

        this_primitiveInitInfo.indexBufferOffset = localIndexBuffer.size();
        this_primitiveInitInfo.pipelineSpecs.indexComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

        AddAccessorDataToLocalBuffer(localIndexBuffer, false, false, sizeof(uint32_t), in_model, this_accessor);
    }

    {
        auto search = in_primitive.attributes.find("POSITION");
        if (search != in_primitive.attributes.end())
        {
            int this_positionAttribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_positionAttribute];

            this_primitiveInitInfo.positionBufferOffset = localPositionBuffer.size();
            this_primitiveInitInfo.pipelineSpecs.positionComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localPositionBuffer, true, false, sizeof(float), in_model, this_accessor);

            if (recordingOBB)
            {
                size_t begin_index_byte = this_primitiveInitInfo.positionBufferOffset;
                size_t end_index_byte = localPositionBuffer.size();

                float* begin_index = reinterpret_cast<float*>(localPositionBuffer.data() + begin_index_byte);
                float* end_index = reinterpret_cast<float*>(localPositionBuffer.data() + end_index_byte);

                for (float* this_ptr = begin_index; this_ptr != end_index; this_ptr += 3)
                    pointsOfRecordingOBB.emplace_back(glm::vec3(this_ptr[0], this_ptr[1], this_ptr[2]));
            }
        }
    }

    {
        auto search = in_primitive.attributes.find("NORMAL");
        if (search != in_primitive.attributes.end())
        {
            int this_normalAttribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_normalAttribute];

            this_primitiveInitInfo.normalBufferOffset = localNormalBuffer.size();
            this_primitiveInitInfo.pipelineSpecs.normalComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localNormalBuffer, false, false, sizeof(float), in_model, this_accessor);
        }
    }

    {
        auto search = in_primitive.attributes.find("TANGENT");
        if (search != in_primitive.attributes.end())
        {
            int this_tangetAttribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_tangetAttribute];

            this_primitiveInitInfo.tangentBufferOffset = localTangentBuffer.size();
            this_primitiveInitInfo.pipelineSpecs.tangentComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localTangentBuffer, false, false, sizeof(float), in_model, this_accessor);
        }
    }

    {
        auto search = in_primitive.attributes.find("TEXCOORD_0");
        if (search != in_primitive.attributes.end())
        {
            int this_texcoord0Attribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_texcoord0Attribute];

            this_primitiveInitInfo.texcoord0BufferOffset = localTexcoord0Buffer.size();
            this_primitiveInitInfo.pipelineSpecs.texcoord0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localTexcoord0Buffer, false, false, sizeof(float), in_model, this_accessor);
        }
    }

    {
        auto search = in_primitive.attributes.find("TEXCOORD_1");
        if (search != in_primitive.attributes.end())
        {
            int this_texcoord1Attribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_texcoord1Attribute];

            this_primitiveInitInfo.texcoord1BufferOffset = localTexcoord1Buffer.size();
            this_primitiveInitInfo.pipelineSpecs.texcoord1ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localTexcoord1Buffer, false, false, sizeof(float), in_model, this_accessor);
        }
    }

    {
        auto search = in_primitive.attributes.find("COLOR_0");
        if (search != in_primitive.attributes.end())
        {
            int this_color0Attribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_color0Attribute];

            this_primitiveInitInfo.color0BufferOffset = localColor0Buffer.size();
            this_primitiveInitInfo.pipelineSpecs.color0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localColor0Buffer, false, true, sizeof(float), in_model, this_accessor);
        }
    }

    primitivesGeneralInfos.emplace_back(this_primitiveInitInfo);
}

void PrimitivesOfMeshes::FlashDevice()
{
    assert(hasBuffersBeenFlashed == false);

    if (!localIndexBuffer.empty())
        indexBuffer_uptr = CreateDeviceBufferForLocalBuffer(localIndexBuffer, Anvil::BufferUsageFlagBits::INDEX_BUFFER_BIT, "Index Buffer");
    if (!localPositionBuffer.empty())
        positionBuffer_uptr = CreateDeviceBufferForLocalBuffer(localPositionBuffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Position Buffer");
    if (!localNormalBuffer.empty())
        normalBuffer_uptr = CreateDeviceBufferForLocalBuffer(localNormalBuffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Normal Buffer");
    if (!localTangentBuffer.empty())
        tangentBuffer_uptr = CreateDeviceBufferForLocalBuffer(localTangentBuffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Tangent Buffer");
    if (!localTexcoord0Buffer.empty())
        texcoord0Buffer_uptr = CreateDeviceBufferForLocalBuffer(localTexcoord0Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Texcoord0 Buffer");
    if (!localTexcoord1Buffer.empty())
        texcoord1Buffer_uptr = CreateDeviceBufferForLocalBuffer(localTexcoord1Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Texcoord1 Buffer");
    if (!localColor0Buffer.empty())
        color0Buffer_uptr = CreateDeviceBufferForLocalBuffer(localColor0Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Color0 Buffer");

    localIndexBuffer.clear();
    localPositionBuffer.clear();
    localNormalBuffer.clear();
    localTangentBuffer.clear();
    localTexcoord0Buffer.clear();
    localTexcoord1Buffer.clear();
    localColor0Buffer.clear();

    hasBuffersBeenFlashed = true;
}

size_t PrimitivesOfMeshes::GetPrimitivesCount()
{
    return primitivesGeneralInfos.size();
}

void PrimitivesOfMeshes::InitPrimitivesSet(PrimitivesSetSpecs in_primitives_set_specs,
                                           const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_lower_descriptorSetCreateInfos,
                                           Anvil::RenderPass* renderpass_ptr,
                                           Anvil::SubPassID subpassID)
{
    std::vector<PrimitiveSpecificSetInfo> this_set_primitiveSpecificSetInfo;

    for (auto& this_primitivesGeneralInfo : primitivesGeneralInfos)
    {
        PrimitiveSpecificSetInfo this_primitiveSpecificSetInfo;

        GraphicsPipelineSpecs this_pipelineSpecs = this_primitivesGeneralInfo.pipelineSpecs;

        std::vector<PushConstantSpecs> this_push_constant_specs;
        PushConstantSpecs modelMatrixPushConstant;
        modelMatrixPushConstant.offset = 0;
        modelMatrixPushConstant.size = sizeof(glm::mat4x4);
        modelMatrixPushConstant.shader_flags = Anvil::ShaderStageFlagBits::VERTEX_BIT;
        this_push_constant_specs.emplace_back(modelMatrixPushConstant);
        this_pipelineSpecs.pushConstantSpecs = this_push_constant_specs;

        ShadersSpecs this_shaderSpecs = in_primitives_set_specs.shaderSpecs;

        int32_t layout_location = 0;
        layout_location++; // position layout_location is 0
      
        std::vector<const Anvil::DescriptorSetCreateInfo*> this_descriptorSetCreateInfos_ptrs = *in_lower_descriptorSetCreateInfos;
        if (in_primitives_set_specs.useMaterial)
        {
            if (this_primitivesGeneralInfo.normalBufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_NORMAL");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_NORMAL_LOCATION", layout_location++));
                this_primitiveSpecificSetInfo.usesNormalBuffer = true;
            }
            if (this_primitivesGeneralInfo.tangentBufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_TANGENT");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TANGENT_LOCATION", layout_location++));
                this_primitiveSpecificSetInfo.usesTangentBuffer = true;
            }
            if (this_primitivesGeneralInfo.texcoord0BufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_TEXCOORD0");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TEXCOORD0_LOCATION", layout_location++));
                this_primitiveSpecificSetInfo.usesTexcoord0Buffer = true;
            }
            if (this_primitivesGeneralInfo.texcoord1BufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_TEXCOORD1");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TEXCOORD1_LOCATION", layout_location++));
                this_primitiveSpecificSetInfo.usesTexcoord1Buffer = true;
            }
            if (this_primitivesGeneralInfo.color0BufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_COLOR0");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_COLOR0_LOCATION", layout_location++));
                this_primitiveSpecificSetInfo.usesColor0Buffer = true;
            }

            this_descriptorSetCreateInfos_ptrs.emplace_back(materialsOfPrimitives_ptr->GetDescriptorSetCreateInfoPtr(this_primitivesGeneralInfo.materialIndex));
            this_primitiveSpecificSetInfo.materialDescriptorSet_ptr = materialsOfPrimitives_ptr->GetDescriptorSetPtr(this_primitivesGeneralInfo.materialIndex);
            
            ShadersSpecs material_shader_specs = materialsOfPrimitives_ptr->GetShaderSpecsNeededForMaterial(this_primitivesGeneralInfo.materialIndex);

            std::copy(
                material_shader_specs.emptyDefinition.begin(),
                material_shader_specs.emptyDefinition.end(),
                std::back_inserter(this_shaderSpecs.emptyDefinition));

            std::copy(
                material_shader_specs.definitionValuePairs.begin(),
                material_shader_specs.definitionValuePairs.end(),
                std::back_inserter(this_shaderSpecs.definitionValuePairs));
        }
        else
        {
            this_pipelineSpecs.normalComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.tangentComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.texcoord0ComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.texcoord1ComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.color0ComponentType = static_cast<glTFcomponentType>(-1);
        }


        this_pipelineSpecs.descriptorSetsCreateInfo_ptrs = std::move(this_descriptorSetCreateInfos_ptrs);
        this_pipelineSpecs.pipelineShaders = shadersSetsFamiliesCache_ptr->GetShadersSet(this_shaderSpecs);

        this_pipelineSpecs.depthCompare = in_primitives_set_specs.depthCompare;
        this_pipelineSpecs.depthWriteEnable = in_primitives_set_specs.useDepthWrite;
        this_pipelineSpecs.renderpass_ptr = renderpass_ptr;
        this_pipelineSpecs.subpassID = subpassID;

        Anvil::PipelineID this_pipelineID = pipelinesFactory_ptr->GetGraphicsPipelineID(this_pipelineSpecs);

        this_primitiveSpecificSetInfo.vkGraphicsPipeline = pipelinesFactory_ptr->GetPipelineVkHandle(Anvil::PipelineBindPoint::GRAPHICS,
                                                                                                          this_pipelineID);
        this_primitiveSpecificSetInfo.pipelineLayout_ptr = pipelinesFactory_ptr->GetPipelineLayout(Anvil::PipelineBindPoint::GRAPHICS, 
                                                                                                        this_pipelineID);

        this_set_primitiveSpecificSetInfo.emplace_back(this_primitiveSpecificSetInfo);
    }

    primitivesSetsNameToVector_umap.emplace(in_primitives_set_specs.primitivesSetName, std::move(this_set_primitiveSpecificSetInfo));
}

void PrimitivesOfMeshes::StartRecordOBB()
{
    recordingOBB = true;
}

OBB PrimitivesOfMeshes::GetOBBandReset()
{
    OBB return_OBB = OBB::CreateOBB(pointsOfRecordingOBB);
    pointsOfRecordingOBB.clear();

    recordingOBB = false;

    return return_OBB;
}

const std::vector<PrimitiveSpecificSetInfo>& PrimitivesOfMeshes::GetPrimitivesSetInfos(std::string in_primitives_set_name) const
{
    auto search = primitivesSetsNameToVector_umap.find(in_primitives_set_name);

    assert(search != primitivesSetsNameToVector_umap.end());

    return search->second;
}

const std::vector<PrimitiveGeneralInfo>& PrimitivesOfMeshes::GetPrimitivesGeneralInfos() const
{
    return primitivesGeneralInfos;
}

void PrimitivesOfMeshes::AddAccessorDataToLocalBuffer(std::vector<unsigned char>& localBuffer_ref,
                                                      bool shouldFlipYZ_position,
                                                      bool vec3_to_vec4_color,
                                                      size_t allignBufferSize,
                                                      const tinygltf::Model& in_model,
                                                      const tinygltf::Accessor& in_accessor) const
{
    size_t count_of_elements = in_accessor.count;
    size_t accessor_byte_offset = in_accessor.byteOffset;

    size_t size_of_each_component_in_byte;
    switch (static_cast<glTFcomponentType>(in_accessor.componentType))
    {
    default:
    case glTFcomponentType::type_byte:
    case glTFcomponentType::type_unsigned_byte:
        size_of_each_component_in_byte = sizeof(int8_t);
        break;
    case glTFcomponentType::type_short:
    case glTFcomponentType::type_unsigned_short:
        size_of_each_component_in_byte = sizeof(int16_t);
        break;
    case glTFcomponentType::type_int:
    case glTFcomponentType::type_unsigned_int:
    case glTFcomponentType::type_float:
        size_of_each_component_in_byte = sizeof(int32_t);
        break;
    case glTFcomponentType::type_double:
        size_of_each_component_in_byte = sizeof(int64_t);
        break;
    }

    size_t number_of_components_per_type;
    switch (static_cast<glTFtype>(in_accessor.type))
    {
    default:
    case glTFtype::type_scalar:
        number_of_components_per_type = 1;
        break;
    case glTFtype::type_vec2:
        number_of_components_per_type = 2;
        break;
    case glTFtype::type_vec3:
        number_of_components_per_type = 3;
        break;
    case glTFtype::type_vec4:
        number_of_components_per_type = 4;
        break;
    }

    const tinygltf::BufferView& this_bufferView = in_model.bufferViews[in_accessor.bufferView];
    size_t bufferview_byte_offset = this_bufferView.byteOffset;

    const tinygltf::Buffer& this_buffer = in_model.buffers[this_bufferView.buffer];

    if (!shouldFlipYZ_position && !(vec3_to_vec4_color && number_of_components_per_type == 3))
        std::copy(&this_buffer.data[bufferview_byte_offset + accessor_byte_offset],
                  &this_buffer.data[bufferview_byte_offset + accessor_byte_offset] + count_of_elements * size_of_each_component_in_byte * number_of_components_per_type,
                  std::back_inserter(localBuffer_ref));
    else if (shouldFlipYZ_position) // position can only be float
    {
        std::unique_ptr<float[]> temp_buffer_uptr;
        temp_buffer_uptr.reset(new float[count_of_elements * number_of_components_per_type]);

        uint8_t* temp_buffer_uint8_ptr;
        temp_buffer_uint8_ptr = reinterpret_cast<unsigned char*>(temp_buffer_uptr.get());

        std::memcpy(temp_buffer_uint8_ptr, &this_buffer.data[bufferview_byte_offset + accessor_byte_offset], count_of_elements * size_of_each_component_in_byte * number_of_components_per_type);

        for (size_t i = 0; i < count_of_elements * number_of_components_per_type; i++)
            if (i % number_of_components_per_type == 1 || i % number_of_components_per_type == 2)
                temp_buffer_uptr[i] *= -1.f;

        std::copy(temp_buffer_uint8_ptr,
                  temp_buffer_uint8_ptr + count_of_elements * size_of_each_component_in_byte * number_of_components_per_type,
                  std::back_inserter(localBuffer_ref));
    }
    else if (vec3_to_vec4_color && number_of_components_per_type == 3) // if color is vec3
    {
        std::unique_ptr<uint8_t[]> temp_buffer_uint8_uptr;
        temp_buffer_uint8_uptr.reset(new uint8_t[count_of_elements * size_of_each_component_in_byte * 4]);

        for (size_t i = 0; i < count_of_elements; i++)
        {
            std::memcpy(temp_buffer_uint8_uptr.get() + size_of_each_component_in_byte * i * 4, &this_buffer.data[bufferview_byte_offset + accessor_byte_offset + i * size_of_each_component_in_byte * 3], size_of_each_component_in_byte * 3);
            if (size_of_each_component_in_byte == 1)
            {
                uint8_t max = -1;
                std::memcpy(temp_buffer_uint8_uptr.get() + size_of_each_component_in_byte * (i * 4 + 3), reinterpret_cast<unsigned char*> (&max), 1);
            }
            else if (size_of_each_component_in_byte == 2)
            {
                uint16_t max = -1;
                std::memcpy(temp_buffer_uint8_uptr.get() + size_of_each_component_in_byte * (i * 4 + 3), reinterpret_cast<unsigned char*> (&max), 2);
            }
            else if (size_of_each_component_in_byte == 4)
            {
                float max = 1.f;
                std::memcpy(temp_buffer_uint8_uptr.get() + size_of_each_component_in_byte * (i * 4 + 3), reinterpret_cast<unsigned char*> (&max), 4);
            }
        }

        std::copy(temp_buffer_uint8_uptr.get(),
                  temp_buffer_uint8_uptr.get() + count_of_elements * size_of_each_component_in_byte * 4,
                  std::back_inserter(localBuffer_ref));

    }
    else
        assert(0);

    if (allignBufferSize != -1)
    {
        const size_t loops_needed_to_align = localBuffer_ref.size() % allignBufferSize;
        for (size_t i = 0; i < loops_needed_to_align; i++)
            localBuffer_ref.emplace_back(0);
    }

}

Anvil::BufferUniquePtr PrimitivesOfMeshes::CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer, Anvil::BufferUsageFlagBits in_bufferusageflag, std::string buffers_name) const
{
    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    in_localBuffer.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    in_bufferusageflag);

    Anvil::BufferUniquePtr buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

    buffer_ptr->set_name(buffers_name);

    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    allocator_ptr->add_buffer(buffer_ptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE);

    buffer_ptr->write(0,
                      in_localBuffer.size(),
                      in_localBuffer.data());

    return std::move(buffer_ptr);
}