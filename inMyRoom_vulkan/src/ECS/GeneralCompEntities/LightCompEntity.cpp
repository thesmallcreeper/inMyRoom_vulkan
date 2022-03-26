#include "ECS/GeneralCompEntities/LightCompEntity.h"

#ifndef GAME_DLL
#include "ECS/GeneralComponents/LightComp.h"
#include "glm/gtc/matrix_inverse.hpp"

LightCompEntity::LightCompEntity(Entity this_entity)
    : CompEntityBaseWrappedClass<LightComp>(this_entity)
{
}

LightCompEntity LightCompEntity::GetEmpty() {
    LightCompEntity this_lightCompEntity(0);

    return this_lightCompEntity;
}

LightCompEntity LightCompEntity::CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap &in_map) {
    LightCompEntity this_lightCompEntity(in_entity);

    // "LightType", lightType = string
    {
        auto search = in_map.stringMap.find("LightType");
        assert(search != in_map.stringMap.end());

        if (search->second == "Sphere")
            this_lightCompEntity.lightType = LightType::Sphere;
        else if (search->second == "Cylinder")
            this_lightCompEntity.lightType = LightType::Cylinder;
        else if (search->second == "Uniform")
            this_lightCompEntity.lightType = LightType::Uniform;
        else if (search->second == "Cone")
            this_lightCompEntity.lightType = LightType::Cone;
    }
    // "Radius", radius = float
    {
        auto search = in_map.floatMap.find("Radius");
        assert(this_lightCompEntity.lightType == LightType::Uniform
            || search != in_map.floatMap.end());

        if (search != in_map.floatMap.end()) {
            this_lightCompEntity.radius = search->second;
        }
    }
    // "Length", length = float
    {
        auto search = in_map.floatMap.find("Length");
        assert(this_lightCompEntity.lightType != LightType::Cylinder
            || search != in_map.floatMap.end());

        if (search != in_map.floatMap.end()) {
            this_lightCompEntity.length = search->second;
        }
    }
    // "Range", range = float
    {
        auto search = in_map.floatMap.find("Range");
        assert(this_lightCompEntity.lightType == LightType::Cone
            || this_lightCompEntity.lightType == LightType::Uniform
            || search != in_map.floatMap.end());

        if (search != in_map.floatMap.end()) {
            this_lightCompEntity.range = search->second;
        }
    }
    // "Luminance", luminance = glm::vec4
    {
        auto search = in_map.vec4Map.find("Luminance");
        assert(search != in_map.vec4Map.end());

        this_lightCompEntity.luminance = {search->second.r,
                                          search->second.g,
                                          search->second.b};
    }

    return this_lightCompEntity;
}

void LightCompEntity::Update(struct Lights *lights_ptr)
{
    if (lightIndex == -1 && not toBeRemoved) {
        lightIndex = lights_ptr->AddLight();
    }
}

void LightCompEntity::ToBeRemovedCallBack(struct Lights *lights_ptr)
{
    if (lightIndex != -1 ) {
        lights_ptr->RemoveLightSafe(lightIndex);

        lightIndex = -1;
        lightOn = false;
        toBeRemoved = true;
    }
}

glm::mat4 LightCompEntity::GetLightMatrix(const glm::mat4 &matrix) const
{
    glm::vec3 z_dir = glm::normalize(glm::vec3(matrix[2]));
    glm::vec3 x_dir = glm::normalize(glm::vec3(matrix[0]) - glm::dot(glm::vec3(matrix[0]) , z_dir) * z_dir);
    glm::vec3 y_dir = glm::cross(z_dir, x_dir);

    glm::mat4 return_matrix;
    if (lightType == LightType::Cylinder)
        return_matrix[0] = glm::vec4(x_dir, 0.f) * length;
    else
        return_matrix[0] = glm::vec4(x_dir, 0.f) * radius;
    return_matrix[1] = glm::vec4(y_dir, 0.f) * radius;
    return_matrix[2] = glm::vec4(z_dir, 0.f) * radius;
    if (lightType == LightType::Cone)
        return_matrix[3] = glm::vec4(-z_dir, 1.f);
    else
        return_matrix[3] = matrix[3];

    return return_matrix;
}

void LightCompEntity::AddLightInfo(const LateNodeGlobalMatrixComp *nodeGlobalMatrix_ptr,
                                   const glm::mat4 &viewport_matrix,
                                   std::vector<ModelMatrices>& model_matrices,
                                   std::vector<LightInfo>& light_infos)
{
    if (lightOn)
    {
        LightInfo this_light_info;
        this_light_info.lightType = lightType;
        this_light_info.radius = radius;
        this_light_info.length = length;
        this_light_info.luminance = luminance;
        this_light_info.range = range;
        this_light_info.lightIndex = lightIndex;

        if (lightType != LightType::Uniform) {
            matricesOffset = model_matrices.size();
            glm::mat4 pos_matrix = GetLightMatrix(viewport_matrix * nodeGlobalMatrix_ptr->GetComponentEntity(thisEntity).globalMatrix);
            glm::mat4 normal_matrix = glm::mat4(glm::vec4(glm::normalize(glm::vec3(pos_matrix[0].x, pos_matrix[0].y, pos_matrix[0].z)), 0.f),
                                                glm::vec4(glm::normalize(glm::vec3(pos_matrix[1].x, pos_matrix[1].y, pos_matrix[1].z)), 0.f),
                                                glm::vec4(glm::normalize(glm::vec3(pos_matrix[2].x, pos_matrix[2].y, pos_matrix[2].z)), 0.f),
                                                glm::vec4(glm::normalize(glm::vec3(pos_matrix[3].x, pos_matrix[3].y, pos_matrix[3].z)), 0.f));
            model_matrices.emplace_back( ModelMatrices({pos_matrix, normal_matrix}));

            this_light_info.matricesOffset = matricesOffset;
        }

        light_infos.emplace_back(this_light_info);
    }
}

bool LightCompEntity::ShouldDraw() const
{
    return (lightOn && lightType != LightType::Uniform);
}

#endif