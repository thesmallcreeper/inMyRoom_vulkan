#include "Graphics/Meshes/PrimitivesOfMeshes.h"

#include <limits>
#include <cassert>
#include <algorithm>
#include <iterator>

#include "misc/memory_allocator.h"
#include "misc/buffer_create_info.h"
#include "misc/base_pipeline_manager.h"

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
    texcoord1Buffer_uptr.reset();
    color0Buffer_uptr.reset();
    joints0Buffer_uptr.reset();
    weights0Buffer_uptr.reset();
}

void PrimitivesOfMeshes::AddPrimitive(const tinygltf::Model& in_model,
                                      const tinygltf::Primitive& in_primitive)
{
    assert(hasBeenFlashed == false);

    PrimitiveGeneralInfo this_primitiveInitInfo;

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
        this_primitiveInitInfo.commonGraphicsPipelineSpecs.indexComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

        AddAccessorDataToLocalBuffer(localIndexBuffer, false, false, sizeof(uint32_t), in_model, this_accessor);
    }

    {
        auto search = in_primitive.attributes.find("POSITION");
        if (search != in_primitive.attributes.end())
        {
            int this_positionAttribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_positionAttribute];

            this_primitiveInitInfo.positionBufferOffset = localPositionBuffer.size();
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.positionComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

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
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.normalComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

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
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.tangentComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

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
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.texcoord0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

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
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.texcoord1ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

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
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.color0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localColor0Buffer, false, true, sizeof(float), in_model, this_accessor);
        }
    }

    {
        auto search = in_primitive.attributes.find("JOINTS_0");
        if (search != in_primitive.attributes.end())
        {
            int this_joints0Attribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_joints0Attribute];

            this_primitiveInitInfo.joints0BufferOffset = localJoints0Buffer.size();
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.joints0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localJoints0Buffer, false, false, sizeof(uint16_t), in_model, this_accessor);
        }
    }

    {
        auto search = in_primitive.attributes.find("WEIGHTS_0");
        if (search != in_primitive.attributes.end())
        {
            int this_weights0Attribute = search->second;
            const tinygltf::Accessor& this_accessor = in_model.accessors[this_weights0Attribute];

            this_primitiveInitInfo.weights0BufferOffset = localWeights0Buffer.size();
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.weights0ComponentType = static_cast<glTFcomponentType>(this_accessor.componentType);

            AddAccessorDataToLocalBuffer(localWeights0Buffer, false, false, sizeof(float), in_model, this_accessor);
        }
    }

    this_primitiveInitInfo.commonGraphicsPipelineSpecs.drawMode = static_cast<glTFmode>(in_primitive.mode);

    if(in_primitive.material != -1)
    {
        this_primitiveInitInfo.materialIndex = static_cast<uint32_t>(in_primitive.material + materialsOfPrimitives_ptr->GetMaterialIndexOffsetOfModel(in_model));
        this_primitiveInitInfo.materialMaps = materialsOfPrimitives_ptr->GetMaterialMapsIndexes(this_primitiveInitInfo.materialIndex);
    }

    
    bool isTransparent = false;

    if(this_primitiveInitInfo.materialIndex != -1)
    {
        MaterialAbout this_materialAbout = materialsOfPrimitives_ptr->GetMaterialAbout(this_primitiveInitInfo.materialIndex);

        if(this_materialAbout.twoSided)
            this_primitiveInitInfo.commonGraphicsPipelineSpecs.twoSided = true;

        // TODO enable blending
        if (this_materialAbout.transparent)
            isTransparent = true;
    }
    

    primitivesGeneralInfos.emplace_back(this_primitiveInitInfo);
    primitivesTransparencyFlags.emplace_back(isTransparent);
}

void PrimitivesOfMeshes::FlashDevice()
{
    assert(hasBeenFlashed == false);

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
    if (!localJoints0Buffer.empty())
        joints0Buffer_uptr = CreateDeviceBufferForLocalBuffer(localJoints0Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Joints0 Buffer");
    if (!localWeights0Buffer.empty())
        weights0Buffer_uptr = CreateDeviceBufferForLocalBuffer(localWeights0Buffer, Anvil::BufferUsageFlagBits::VERTEX_BUFFER_BIT, "Weights0 Buffer");


    localIndexBuffer.clear();
    localPositionBuffer.clear();
    localNormalBuffer.clear();
    localTangentBuffer.clear();
    localTexcoord0Buffer.clear();
    localTexcoord1Buffer.clear();
    localColor0Buffer.clear();
    localJoints0Buffer.clear();
    localWeights0Buffer.clear();

    hasBeenFlashed = true;
}

size_t PrimitivesOfMeshes::GetPrimitivesCount()
{
    return primitivesGeneralInfos.size();
}

bool PrimitivesOfMeshes::IsPrimitiveTransparent(size_t primitive_index)
{
    return primitivesTransparencyFlags[primitive_index];
}

void PrimitivesOfMeshes::InitPrimitivesSet(PrimitivesSetSpecs in_primitives_set_specs,
                                           DescriptorSetsCreateInfosPtrsCollection in_descriptor_sets_create_infos_ptrs_collection,
                                           Anvil::RenderPass* renderpass_ptr,
                                           Anvil::SubPassID subpassID)
{
    std::vector<PrimitiveSpecificSetInfo> this_set_primitiveSpecificSetInfo;

    for (auto& this_primitivesGeneralInfo : primitivesGeneralInfos)
    {
        PrimitiveSpecificSetInfo this_primitiveSpecificSetInfo;

        GraphicsPipelineSpecs this_set_graphicsPipelineSpecs = this_primitivesGeneralInfo.commonGraphicsPipelineSpecs;        // It is getting filled up from the properties of the primitive
        ShadersSpecs this_set_shaderSpecs = in_primitives_set_specs.shaderSpecs;                                              // It is getting filled up from the properties of the primitive

        {   // Index data
            this_primitiveSpecificSetInfo.indexBufferType = this_primitivesGeneralInfo.indexBufferType;
            this_primitiveSpecificSetInfo.indicesCount = this_primitivesGeneralInfo.indicesCount;

            this_primitiveSpecificSetInfo.indexBufferOffset = this_primitivesGeneralInfo.indexBufferOffset;
        }
        
        {   // Description set 0 is always the camera location
            this_set_graphicsPipelineSpecs.descriptorSetsCreateInfo_ptrs.emplace_back(in_descriptor_sets_create_infos_ptrs_collection.camera_description_set_create_info_ptr);
        }

        if (this_primitivesGeneralInfo.joints0BufferOffset != -1 && this_primitivesGeneralInfo.weights0BufferOffset != -1)
        {
            this_set_graphicsPipelineSpecs.descriptorSetsCreateInfo_ptrs.emplace_back(in_descriptor_sets_create_infos_ptrs_collection.skins_description_set_create_info_ptr);
        }

        {   // Push constants of pipeline
            if(this_primitivesGeneralInfo.joints0BufferOffset == -1 || this_primitivesGeneralInfo.weights0BufferOffset == -1)
            {
                PushConstantSpecs TRSmatrixPushConstant;
                TRSmatrixPushConstant.offset = 0;
                TRSmatrixPushConstant.size = 64;
                TRSmatrixPushConstant.shader_flags = Anvil::ShaderStageFlagBits::VERTEX_BIT;
                this_set_graphicsPipelineSpecs.pushConstantSpecs.emplace_back(TRSmatrixPushConstant);
            }
            else
            {
                PushConstantSpecs SkinInfoPushConstant;
                SkinInfoPushConstant.offset = 0;
                SkinInfoPushConstant.size = 8;
                SkinInfoPushConstant.shader_flags = Anvil::ShaderStageFlagBits::VERTEX_BIT;
                this_set_graphicsPipelineSpecs.pushConstantSpecs.emplace_back(SkinInfoPushConstant);
            }

            if(in_primitives_set_specs.useMaterial && this_primitivesGeneralInfo.materialIndex != -1)
            {
                PushConstantSpecs materialPushConstant;
                materialPushConstant.offset = 96;
                materialPushConstant.size = 32;
                materialPushConstant.shader_flags = Anvil::ShaderStageFlagBits::FRAGMENT_BIT;
                this_set_graphicsPipelineSpecs.pushConstantSpecs.emplace_back(materialPushConstant);
            }
        }

        {   // Vertex attributes
            int32_t layout_location = 0;

            {
                layout_location++;      // position layout_location is 0
                this_primitiveSpecificSetInfo.positionBufferOffset = this_primitivesGeneralInfo.positionBufferOffset;
            }

            if (in_primitives_set_specs.useMaterial)
            {
                if (this_primitivesGeneralInfo.normalBufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_NORMAL");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_NORMAL_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.normalBufferOffset = this_primitivesGeneralInfo.normalBufferOffset;
                }
                if (this_primitivesGeneralInfo.tangentBufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_TANGENT");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TANGENT_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.tangentBufferOffset = this_primitivesGeneralInfo.tangentBufferOffset;
                }
                if (this_primitivesGeneralInfo.texcoord0BufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_TEXCOORD0");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TEXCOORD0_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.texcoord0BufferOffset = this_primitivesGeneralInfo.texcoord0BufferOffset;

                    this_primitiveSpecificSetInfo.texcoord0ComponentType = this_primitivesGeneralInfo.commonGraphicsPipelineSpecs.texcoord0ComponentType;
                }
                if (this_primitivesGeneralInfo.texcoord1BufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_TEXCOORD1");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_TEXCOORD1_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.texcoord1BufferOffset = this_primitivesGeneralInfo.texcoord1BufferOffset;

                    this_primitiveSpecificSetInfo.texcoord1ComponentType = this_primitivesGeneralInfo.commonGraphicsPipelineSpecs.texcoord1ComponentType;
                }
                if (this_primitivesGeneralInfo.color0BufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_COLOR0");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_COLOR0_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.color0BufferOffset = this_primitivesGeneralInfo.color0BufferOffset;

                    this_primitiveSpecificSetInfo.color0ComponentType = this_primitivesGeneralInfo.commonGraphicsPipelineSpecs.color0ComponentType;
                }
                if (this_primitivesGeneralInfo.joints0BufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_JOINTS0");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_JOINTS0_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.joints0BufferOffset = this_primitivesGeneralInfo.joints0BufferOffset;

                    this_primitiveSpecificSetInfo.joints0ComponentType = this_primitivesGeneralInfo.commonGraphicsPipelineSpecs.joints0ComponentType;
                }
                if (this_primitivesGeneralInfo.weights0BufferOffset != -1)
                {
                    this_set_shaderSpecs.emptyDefinition.emplace_back("VERT_WEIGHTS0");
                    this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("VERT_WEIGHTS0_LOCATION", layout_location++));
                    this_primitiveSpecificSetInfo.weights0BufferOffset = this_primitivesGeneralInfo.weights0BufferOffset;

                    this_primitiveSpecificSetInfo.weights0ComponentType = this_primitivesGeneralInfo.commonGraphicsPipelineSpecs.weights0ComponentType;
                }
                if (this_primitivesGeneralInfo.weights0BufferOffset != -1 && this_primitivesGeneralInfo.joints0BufferOffset != -1)
                { 
                    this_set_shaderSpecs.emptyDefinition.emplace_back("USE_SKIN");
                }

                {
                    ShadersSpecs material_shader_specs = materialsOfPrimitives_ptr->GetShaderSpecsNeededForMaterial(this_primitivesGeneralInfo.materialIndex);

                    std::copy(
                        material_shader_specs.emptyDefinition.begin(),
                        material_shader_specs.emptyDefinition.end(),
                        std::back_inserter(this_set_shaderSpecs.emptyDefinition));

                    std::copy(
                        material_shader_specs.definitionValuePairs.begin(),
                        material_shader_specs.definitionValuePairs.end(),
                        std::back_inserter(this_set_shaderSpecs.definitionValuePairs));

                    std::copy(
                        material_shader_specs.definitionStringPairs.begin(),
                        material_shader_specs.definitionStringPairs.end(),
                        std::back_inserter(this_set_shaderSpecs.definitionStringPairs));
                }


                // Add material Description set
                if (this_primitivesGeneralInfo.materialIndex != -1)
                {
                    this_set_graphicsPipelineSpecs.descriptorSetsCreateInfo_ptrs.emplace_back(in_descriptor_sets_create_infos_ptrs_collection.materials_description_set_create_info_ptr);
                    if (this_primitivesGeneralInfo.joints0BufferOffset == -1 || this_primitivesGeneralInfo.weights0BufferOffset == -1)
                        this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("MATERIAL_DS_INDEX", 1));
                    else
                        this_set_shaderSpecs.definitionValuePairs.emplace_back(std::make_pair("MATERIAL_DS_INDEX", 2));
                }
            }
            else
            {
                this_set_graphicsPipelineSpecs.normalComponentType = static_cast<glTFcomponentType>(-1);
                this_set_graphicsPipelineSpecs.tangentComponentType = static_cast<glTFcomponentType>(-1);
                this_set_graphicsPipelineSpecs.texcoord0ComponentType = static_cast<glTFcomponentType>(-1);
                this_set_graphicsPipelineSpecs.texcoord1ComponentType = static_cast<glTFcomponentType>(-1);
                this_set_graphicsPipelineSpecs.color0ComponentType = static_cast<glTFcomponentType>(-1);
            }
        }

        this_set_graphicsPipelineSpecs.pipelineShaders = shadersSetsFamiliesCache_ptr->GetShadersSet(this_set_shaderSpecs);

        this_set_graphicsPipelineSpecs.depthCompare = in_primitives_set_specs.depthCompare;
        this_set_graphicsPipelineSpecs.depthWriteEnable = in_primitives_set_specs.useDepthWrite;

        this_set_graphicsPipelineSpecs.renderpass_ptr = renderpass_ptr;
        this_set_graphicsPipelineSpecs.subpassID = subpassID;

        Anvil::PipelineID this_pipelineID = pipelinesFactory_ptr->GetGraphicsPipelineID(this_set_graphicsPipelineSpecs);

        this_primitiveSpecificSetInfo.vkGraphicsPipeline = pipelinesFactory_ptr->GetPipelineVkHandle(Anvil::PipelineBindPoint::GRAPHICS,
                                                                                                     this_pipelineID);
        this_primitiveSpecificSetInfo.pipelineLayout_ptr = pipelinesFactory_ptr->GetPipelineLayout(Anvil::PipelineBindPoint::GRAPHICS, 
                                                                                                   this_pipelineID);
        
        if (in_primitives_set_specs.useMaterial)
        {
            this_primitiveSpecificSetInfo.materialIndex = this_primitivesGeneralInfo.materialIndex;
            this_primitiveSpecificSetInfo.materialMaps = this_primitivesGeneralInfo.materialMaps;
        }

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

// ultimate shitty coding below
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

Anvil::BufferUniquePtr PrimitivesOfMeshes::CreateDeviceBufferForLocalBuffer(const std::vector<unsigned char>& in_localBuffer,
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
