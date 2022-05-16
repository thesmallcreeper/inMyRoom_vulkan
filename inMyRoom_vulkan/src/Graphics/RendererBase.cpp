#include "Graphics/RendererBase.h"

#include "Graphics/Graphics.h"

std::vector<RendererBase::PrimitiveInstanceParameters> RendererBase::CreatePrimitivesInstanceParameters()
{
    std::vector<PrimitiveInstanceParameters> return_vector;

    PrimitiveInstanceParameters default_instance_parameters = {};
    default_instance_parameters.material = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().material;
    default_instance_parameters.matricesOffset = 0;
    default_instance_parameters.prevMatricesOffset = -1;
    default_instance_parameters.indicesOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().indicesByteOffset / sizeof(uint32_t);
    default_instance_parameters.positionOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().positionByteOffset / sizeof(glm::vec4);
    default_instance_parameters.normalOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().normalByteOffset / sizeof(glm::vec4);
    default_instance_parameters.tangentOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().tangentByteOffset / sizeof(glm::vec4);
    default_instance_parameters.texcoordsOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().texcoordsByteOffset / sizeof(glm::vec2);
    default_instance_parameters.colorOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().colorByteOffset / sizeof(glm::vec4);
    return_vector.emplace_back(default_instance_parameters);

    for (DrawInfo& this_draw_info : drawInfos) {
        this_draw_info.primitivesInstanceOffset = return_vector.size();

        std::vector<PrimitiveInfo> primitives_info;
        std::vector<DynamicMeshInfo::DynamicPrimitiveInfo> dynamic_primitives_info;
        uint32_t descriptor_index = 0;
        if (this_draw_info.dynamicMeshIndex != -1) {
            descriptor_index = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex).descriptorIndexOffset + 1;
            dynamic_primitives_info = graphics_ptr->GetDynamicMeshes()->GetDynamicMeshInfo(this_draw_info.dynamicMeshIndex).dynamicPrimitives;
            for (const auto& this_dynamic_primitive_info: dynamic_primitives_info) {
                const PrimitiveInfo &this_primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(this_dynamic_primitive_info.primitiveIndex);
                primitives_info.emplace_back(this_primitive_info);
            }
        } else {
            for (size_t primitive_index : graphics_ptr->GetMeshesOfNodesPtr()->GetMeshInfo(this_draw_info.meshIndex).primitivesIndex) {
                const PrimitiveInfo& primitive_info = graphics_ptr->GetPrimitivesOfMeshes()->GetPrimitiveInfo(primitive_index);
                primitives_info.emplace_back(primitive_info);
                dynamic_primitives_info.emplace_back();
            }
        }

        for (size_t i = 0; i != primitives_info.size(); ++i) {
            PrimitiveInstanceParameters this_primitiveInstanceParameters = {};

            this_primitiveInstanceParameters.material = primitives_info[i].material;
            this_primitiveInstanceParameters.matricesOffset = this_draw_info.matricesOffset;
            this_primitiveInstanceParameters.prevMatricesOffset = this_draw_info.prevMatricesOffset;

            if (primitives_info[i].drawMode == vk::PrimitiveTopology::eTriangleList)
                this_primitiveInstanceParameters.indicesSetMultiplier = 3;
            else if (primitives_info[i].drawMode == vk::PrimitiveTopology::eLineList)
                this_primitiveInstanceParameters.indicesSetMultiplier = 2;
            else
                this_primitiveInstanceParameters.indicesSetMultiplier = 1;

            this_primitiveInstanceParameters.indicesOffset = primitives_info[i].indicesByteOffset / sizeof(uint32_t);

            if (dynamic_primitives_info[i].positionByteOffset != -1) {
                this_primitiveInstanceParameters.positionOffset = dynamic_primitives_info[i].positionByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.positionDescriptorIndex = descriptor_index;
            } else {
                this_primitiveInstanceParameters.positionOffset = primitives_info[i].positionByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.positionDescriptorIndex = 0;
            }

            if (dynamic_primitives_info[i].normalByteOffset != -1) {
                this_primitiveInstanceParameters.normalOffset = dynamic_primitives_info[i].normalByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.normalDescriptorIndex = descriptor_index;
            } else {
                assert(this_draw_info.isLightSource || primitives_info[i].normalByteOffset != -1);
                this_primitiveInstanceParameters.normalOffset = primitives_info[i].normalByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.normalDescriptorIndex = 0;
            }

            if (dynamic_primitives_info[i].tangentByteOffset != -1) {
                this_primitiveInstanceParameters.tangentOffset = dynamic_primitives_info[i].tangentByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.tangentDescriptorIndex = descriptor_index;
            } else {
                assert(this_draw_info.isLightSource || primitives_info[i].tangentByteOffset != -1);
                this_primitiveInstanceParameters.tangentOffset = primitives_info[i].tangentByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.tangentDescriptorIndex = 0;
            }

            if (dynamic_primitives_info[i].texcoordsByteOffset != -1) {
                this_primitiveInstanceParameters.texcoordsStepMultiplier = dynamic_primitives_info[i].texcoordsCount;
                this_primitiveInstanceParameters.texcoordsOffset = dynamic_primitives_info[i].texcoordsByteOffset / sizeof(glm::vec2);
                this_primitiveInstanceParameters.texcoordsDescriptorIndex = descriptor_index;
            } else {
                if (primitives_info[i].texcoordsByteOffset != -1) {
                    this_primitiveInstanceParameters.texcoordsStepMultiplier = primitives_info[i].texcoordsCount;
                    this_primitiveInstanceParameters.texcoordsOffset = primitives_info[i].texcoordsByteOffset / sizeof(glm::vec2);
                    this_primitiveInstanceParameters.texcoordsDescriptorIndex = 0;
                } else {
                    this_primitiveInstanceParameters.texcoordsStepMultiplier = 0;
                    this_primitiveInstanceParameters.texcoordsOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().texcoordsByteOffset / sizeof(glm::vec2);
                    this_primitiveInstanceParameters.texcoordsDescriptorIndex = 0;
                }
            }

            if (dynamic_primitives_info[i].colorByteOffset != -1) {
                this_primitiveInstanceParameters.colorStepMultiplier = 1;
                this_primitiveInstanceParameters.colorOffset = dynamic_primitives_info[i].colorByteOffset / sizeof(glm::vec4);
                this_primitiveInstanceParameters.colorDescriptorIndex = descriptor_index;
            } else {
                if (primitives_info[i].colorByteOffset != -1) {
                    this_primitiveInstanceParameters.colorStepMultiplier = 1;
                    this_primitiveInstanceParameters.colorOffset = primitives_info[i].colorByteOffset / sizeof(glm::vec4);
                    this_primitiveInstanceParameters.colorDescriptorIndex = 0;
                } else {
                    this_primitiveInstanceParameters.colorStepMultiplier = 0;
                    this_primitiveInstanceParameters.colorOffset = graphics_ptr->GetPrimitivesOfMeshes()->GetDefaultPrimitiveInfo().colorByteOffset / sizeof(glm::vec4);
                    this_primitiveInstanceParameters.colorDescriptorIndex = 0;
                }
            }

            if (this_draw_info.isLightSource) {
                auto light_offset = uint16_t(graphics_ptr->GetLights()->GetLightInfo(this_draw_info.lightIndex).lightOffset);
                this_primitiveInstanceParameters.light = light_offset;

                this_primitiveInstanceParameters.lightsCombinationsOffset = 0;
                this_primitiveInstanceParameters.lightsCombinationsCount = 0;
            } else {
                this_primitiveInstanceParameters.light = -1;

                LightsIndicesRange lights_range_indices;
                glm::mat4 pos_matrix = matrices[this_draw_info.matricesOffset].positionMatrix;
                if (this_draw_info.dynamicMeshIndex != -1) {
                    lights_range_indices = graphics_ptr->GetLights()->CreateCollidedLightsRange(pos_matrix * dynamic_primitives_info[i].dynamicPrimitiveOBB);
                } else {
                    lights_range_indices = graphics_ptr->GetLights()->CreateCollidedLightsRange(pos_matrix * primitives_info[i].primitiveOBB);
                }

                this_primitiveInstanceParameters.lightsCombinationsOffset = lights_range_indices.offset;
                this_primitiveInstanceParameters.lightsCombinationsCount = lights_range_indices.size;
            }

            return_vector.emplace_back(this_primitiveInstanceParameters);
        }
    }

    return return_vector;
}
