#include "MeshesPrimitives.h"

#include <cassert>
#include <algorithm>
#include <iterator>

MeshesPrimitives::MeshesPrimitives(PrimitivesPipelines* in_primitivesPipelines_ptr,
                                   PrimitivesShaders* in_primitivesShaders_ptr,
                                   PrimitivesMaterials* in_primitivesMaterials_ptr,
                                   Anvil::BaseDevice* const in_device_ptr)
    :
    primitivesPipelines_ptr(in_primitivesPipelines_ptr),
    primitivesShaders_ptr(in_primitivesShaders_ptr),
    primitivesMaterials_ptr(in_primitivesMaterials_ptr),
    device_ptr(in_device_ptr)
{
}

MeshesPrimitives::~MeshesPrimitives()
{
    indexBuffer_uptr.reset();
    positionBuffer_uptr.reset();
    normalBuffer_uptr.reset();
    tangentBuffer_uptr.reset();
    texcoord0Buffer_uptr.reset();
}

void MeshesPrimitives::AddPrimitive(tinygltf::Model& in_model, tinygltf::Primitive& in_primitive)
{
    assert(hasBuffersBeenFlashed == false);

    PrimitiveInitInfo this_primitiveInitInfo;
    this_primitiveInitInfo.pipelineSpecs.drawMode = static_cast<glTFmode>(in_primitive.mode);

    {
        tinygltf::Accessor& this_accessor = in_model.accessors[in_primitive.indices];

        this_primitiveInitInfo.indicesCount = static_cast<uint32_t>(this_accessor.count);

        if (this_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_short))
            this_primitiveInitInfo.indexBufferType = Anvil::IndexType::UINT16;
        else if (this_accessor.componentType == static_cast<int>(glTFcomponentType::type_unsigned_int))
            this_primitiveInitInfo.indexBufferType = Anvil::IndexType::UINT32;
        else
            assert(0);

        this_primitiveInitInfo.indexBufferOffset = localIndexBuffer.size();

        AddAccessorDataToLocalBuffer(localIndexBuffer, false, in_model, this_accessor);

        this_primitiveInitInfo.pipelineSpecs.indexComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);
    }

    {
        auto search = in_primitive.attributes.find("POSITION");
        if (search != in_primitive.attributes.end())
        {
            int this_positionAttribute = search->second;
            tinygltf::Accessor& this_accessor = in_model.accessors[this_positionAttribute];

            if (this_accessor.componentType != static_cast<int>(glTFcomponentType::type_float))
                assert(0);

            this_primitiveInitInfo.positionBufferOffset = localPositionBuffer.size();

            AddAccessorDataToLocalBuffer(localPositionBuffer, true, in_model, this_accessor);

            this_primitiveInitInfo.pipelineSpecs.positionComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);
        }
    }

    {
        auto search = in_primitive.attributes.find("NORMAL");
        if (search != in_primitive.attributes.end())
        {
            int this_normalAttribute = search->second;
            tinygltf::Accessor& this_accessor = in_model.accessors[this_normalAttribute];

            if (this_accessor.componentType != static_cast<int>(glTFcomponentType::type_float))
                assert(0);

            this_primitiveInitInfo.normalBufferOffset = localNormalBuffer.size();

            AddAccessorDataToLocalBuffer(localNormalBuffer, false, in_model, this_accessor);

            this_primitiveInitInfo.pipelineSpecs.normalComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);
        }
    }

    {
        auto search = in_primitive.attributes.find("TANGENT");
        if (search != in_primitive.attributes.end())
        {
            int this_tangetAttribute = search->second;
            tinygltf::Accessor& this_accessor = in_model.accessors[this_tangetAttribute];

            if (this_accessor.componentType != static_cast<int>(glTFcomponentType::type_float))
                assert(0);

            this_primitiveInitInfo.tangentBufferOffset = localTangentBuffer.size();

            AddAccessorDataToLocalBuffer(localTangentBuffer, false, in_model, this_accessor);

            this_primitiveInitInfo.pipelineSpecs.tangentComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);
        }
    }

    {
        auto search = in_primitive.attributes.find("TEXCOORD_0");
        if (search != in_primitive.attributes.end())
        {
            int this_texcoord0Attribute = search->second;
            tinygltf::Accessor& this_accessor = in_model.accessors[this_texcoord0Attribute];

            if (this_accessor.componentType != static_cast<int>(glTFcomponentType::type_float)
                && this_accessor.componentType != static_cast<int>(glTFcomponentType::type_unsigned_byte)
                && this_accessor.componentType != static_cast<int>(glTFcomponentType::type_unsigned_short))
                assert(0);

            this_primitiveInitInfo.texcoord0BufferOffset = localTexcoord0Buffer.size();

            AddAccessorDataToLocalBuffer(localTexcoord0Buffer, false, in_model, this_accessor);

            this_primitiveInitInfo.pipelineSpecs.texcoord0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);
        }
    }

    {
        auto search = in_primitive.attributes.find("TEXCOORD_1");
        if (search != in_primitive.attributes.end())
        {
            int this_texcoord1Attribute = search->second;
            tinygltf::Accessor& this_accessor = in_model.accessors[this_texcoord1Attribute];

            if (this_accessor.componentType != static_cast<int>(glTFcomponentType::type_float)
                && this_accessor.componentType != static_cast<int>(glTFcomponentType::type_unsigned_byte)
                && this_accessor.componentType != static_cast<int>(glTFcomponentType::type_unsigned_short))
                assert(0);

            this_primitiveInitInfo.texcoord1BufferOffset = localTexcoord1Buffer.size();

            AddAccessorDataToLocalBuffer(localTexcoord1Buffer, false, in_model, this_accessor);

            this_primitiveInitInfo.pipelineSpecs.texcoord1ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);
        }
    }

    this_primitiveInitInfo.materialIndex = in_primitive.material;

    primitivesInitInfos.emplace_back(this_primitiveInitInfo);
}

void MeshesPrimitives::FlashBuffersToDevice()
{
    assert(hasBuffersBeenFlashed == false);

    if (!localIndexBuffer.empty())
        indexBuffer_uptr = CreateDeviceBufferForLocalBuffer(localIndexBuffer, Anvil::BufferUsageFlagBits::INDEX_BUFFER_BIT);
    if (!localPositionBuffer.empty())
        positionBuffer_uptr = CreateDeviceBufferForLocalBuffer(localPositionBuffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);
    if (!localNormalBuffer.empty())
        normalBuffer_uptr = CreateDeviceBufferForLocalBuffer(localNormalBuffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);
    if (!localTangentBuffer.empty())
        tangentBuffer_uptr = CreateDeviceBufferForLocalBuffer(localTangentBuffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);
    if (!localTexcoord0Buffer.empty())
        texcoord0Buffer_uptr = CreateDeviceBufferForLocalBuffer(localTexcoord0Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);
    if (!localTexcoord1Buffer.empty())
        texcoord1Buffer_uptr = CreateDeviceBufferForLocalBuffer(localTexcoord1Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT);

    hasBuffersBeenFlashed = true;
}


size_t MeshesPrimitives::InitPrimitivesSet(ShadersSpecs in_shader_specs, bool use_material,
                                           const std::vector<const Anvil::DescriptorSetCreateInfo*>* in_lower_descriptorSetCreateInfos,
                                           Anvil::RenderPass* renderpass_ptr, Anvil::SubPassID subpassID)
{
    std::vector<PrimitiveInfo> this_set_primitiveInfo;

    for (auto& this_primitivesInitInfo : primitivesInitInfos)
    {
        PrimitiveInfo this_primitiveInfo;
        ShadersSpecs this_shaderSpecs = in_shader_specs;

        int32_t layout_location = 0;

        this_primitiveInfo.indexBufferType = this_primitivesInitInfo.indexBufferType;
        this_primitiveInfo.indicesCount = this_primitivesInitInfo.indicesCount;
        this_primitiveInfo.indexBufferOffset = this_primitivesInitInfo.indexBufferOffset;

        PipelineSpecs this_pipelineSpecs = this_primitivesInitInfo.pipelineSpecs;

        if (this_primitivesInitInfo.positionBufferOffset != -1)
        {
            this_shaderSpecs.emptyDefinition.emplace_back("VERT_POSITION");
            this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_POSITION_LOCATION", layout_location++));
            this_primitiveInfo.positionBufferOffset = this_primitivesInitInfo.positionBufferOffset;
        }
        else
        {
            this_pipelineSpecs.positionComponentType = static_cast<glTFcomponentType>(-1);
        }


        std::vector<const Anvil::DescriptorSetCreateInfo*> this_descriptorSetCreateInfos_ptrs = *
            in_lower_descriptorSetCreateInfos;
        if (this_primitivesInitInfo.materialIndex != -1 && use_material)
        {
            if (this_primitivesInitInfo.normalBufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_NORMAL");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_NORMAL_LOCATION", layout_location++));
                this_primitiveInfo.normalBufferOffset = this_primitivesInitInfo.normalBufferOffset;
            }
            if (this_primitivesInitInfo.tangentBufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_TANGENT");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TANGENT_LOCATION", layout_location++));
                this_primitiveInfo.tangentBufferOffset = this_primitivesInitInfo.tangentBufferOffset;
            }
            if (this_primitivesInitInfo.texcoord0BufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_TEXCOORD0");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TEXCOORD0_LOCATION", layout_location++));
                this_primitiveInfo.texcoord0BufferOffset = this_primitivesInitInfo.texcoord0BufferOffset;
            }
            if (this_primitivesInitInfo.texcoord1BufferOffset != -1)
            {
                this_shaderSpecs.emptyDefinition.emplace_back("VERT_TEXCOORD1");
                this_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TEXCOORD1_LOCATION", layout_location++));
                this_primitiveInfo.texcoord1BufferOffset = this_primitivesInitInfo.texcoord1BufferOffset;
            }

            this_descriptorSetCreateInfos_ptrs.emplace_back(primitivesMaterials_ptr->texturesOfMaterialsDescriptorSetGroup_uptr->get_descriptor_set_create_info(static_cast<uint32_t>(this_primitivesInitInfo.materialIndex)));
            this_primitiveInfo.materialDescriptorSet_ptr = primitivesMaterials_ptr->texturesOfMaterialsDescriptorSetGroup_uptr->get_descriptor_set(static_cast<uint32_t>(this_primitivesInitInfo.materialIndex));

            std::copy(
                primitivesMaterials_ptr->materialsShadersSpecs[this_primitivesInitInfo.materialIndex].emptyDefinition.begin(),
                primitivesMaterials_ptr->materialsShadersSpecs[this_primitivesInitInfo.materialIndex].emptyDefinition.end(),
                std::back_inserter(this_shaderSpecs.emptyDefinition));

            std::copy(
                primitivesMaterials_ptr->materialsShadersSpecs[this_primitivesInitInfo.materialIndex].definitionValuePairs.begin(),
                primitivesMaterials_ptr->materialsShadersSpecs[this_primitivesInitInfo.materialIndex].definitionValuePairs.end(),
                std::back_inserter(this_shaderSpecs.definitionValuePairs));
        }
        else
        {
            this_pipelineSpecs.normalComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.tangentComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.texcoord0ComponentType = static_cast<glTFcomponentType>(-1);
            this_pipelineSpecs.texcoord1ComponentType = static_cast<glTFcomponentType>(-1);
        }

        size_t shaderSet_index = primitivesShaders_ptr->GetShaderSetIndex(this_shaderSpecs);

        this_pipelineSpecs.descriptorSetsCreateInfo_ptrs = std::move(this_descriptorSetCreateInfos_ptrs);

        this_pipelineSpecs.pipelineShaders = primitivesShaders_ptr->shadersSets[shaderSet_index];
        this_pipelineSpecs.renderpass_ptr = renderpass_ptr;
        this_pipelineSpecs.subpassID = subpassID;

		Anvil::PipelineID this_pipelineID = primitivesPipelines_ptr->GetPipelineID(this_pipelineSpecs);
		auto gfx_manager_ptr(device_ptr->get_graphics_pipeline_manager());

		this_primitiveInfo.thisPipelineID = this_pipelineID;
		this_primitiveInfo.pipelineLayout_ptr = gfx_manager_ptr->get_pipeline_layout(this_pipelineID);

        this_set_primitiveInfo.emplace_back(this_primitiveInfo);
    }

    primitivesSets.emplace_back(std::move(this_set_primitiveInfo));
    return primitivesSets.size() - 1;
}

void MeshesPrimitives::AddAccessorDataToLocalBuffer(std::vector<unsigned char>& localBuffer_ref, bool itIsPositionData,
                                                    tinygltf::Model& in_model, tinygltf::Accessor in_accessor) const
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

    tinygltf::BufferView& this_bufferView = in_model.bufferViews[in_accessor.bufferView];
    size_t bufferview_byte_offset = this_bufferView.byteOffset;

    tinygltf::Buffer& this_buffer = in_model.buffers[this_bufferView.buffer];

	if(!itIsPositionData)
	    std::copy(&this_buffer.data[bufferview_byte_offset + accessor_byte_offset],
	              &this_buffer.data[bufferview_byte_offset + accessor_byte_offset] + count_of_elements * size_of_each_component_in_byte * number_of_components_per_type,
	              std::back_inserter(localBuffer_ref));
	else
	{
		std::unique_ptr<float[]> temp_buffer_uptr;
		temp_buffer_uptr.reset(new float[count_of_elements * number_of_components_per_type]);

		uint8_t* temp_buffer_uint8_ptr;
		temp_buffer_uint8_ptr = reinterpret_cast<unsigned char*>(temp_buffer_uptr.get());

		std::memcpy(temp_buffer_uint8_ptr, &this_buffer.data[bufferview_byte_offset + accessor_byte_offset], count_of_elements * size_of_each_component_in_byte * number_of_components_per_type);

		for (size_t i = 0; i < count_of_elements * number_of_components_per_type; i++)
			if (i % 3 == 1 || i % 3 == 2)
				temp_buffer_uptr[i] *= -1.f;

		std::copy(temp_buffer_uint8_ptr,
			      temp_buffer_uint8_ptr + count_of_elements * size_of_each_component_in_byte * number_of_components_per_type,
			      std::back_inserter(localBuffer_ref));
	}

}

Anvil::BufferUniquePtr MeshesPrimitives::CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer, Anvil::BufferUsageFlagBits in_bufferusageflag) const
{
    auto create_info_ptr = Anvil::BufferCreateInfo::create_no_alloc(device_ptr,
                                                                    in_localBuffer.size(),
                                                                    Anvil::QueueFamilyFlagBits::GRAPHICS_BIT,
                                                                    Anvil::SharingMode::EXCLUSIVE,
                                                                    Anvil::BufferCreateFlagBits::NONE,
                                                                    in_bufferusageflag);

    Anvil::BufferUniquePtr buffer_ptr = Anvil::Buffer::create(std::move(create_info_ptr));

    auto allocator_ptr = Anvil::MemoryAllocator::create_oneshot(device_ptr);

    allocator_ptr->add_buffer(buffer_ptr.get(),
                              Anvil::MemoryFeatureFlagBits::NONE);

    buffer_ptr->write(0,
                      in_localBuffer.size(),
                      in_localBuffer.data());

    return std::move(buffer_ptr);
}
