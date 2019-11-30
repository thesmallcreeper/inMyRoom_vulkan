#include "ECS/GeneralCompEntities/PositionCompEntity.h"

#include "ECS/GeneralComponents/PositionComp.h"

PositionComp* PositionCompEntity::positionComp_ptr = nullptr;

PositionCompEntity::PositionCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

PositionCompEntity::~PositionCompEntity()
{
}

PositionCompEntity PositionCompEntity::GetEmpty()
{
    PositionCompEntity this_positionCompEntity(0);
    this_positionCompEntity.localScale = glm::vec3(1.f, 1.f, 1.f);
    this_positionCompEntity.localRotation = glm::qua<float>(1.f, 0.f, 0.f, 0.f);
    this_positionCompEntity.localTranslation = glm::vec3(0.f, 0.f, 0.f);

    return this_positionCompEntity;
}

PositionCompEntity PositionCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map)
{
    PositionCompEntity this_positionCompEntity(in_entity);

    // "LocalScale", localScale.xyz = vec4.xyz              (optional)
    {
        auto search = in_map.vec4Map.find("LocalScale");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_positionCompEntity.localScale = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }
    // "LocalRotation", localRotation.xyzw = vec4.xyzw      (optional)
    {
        auto search = in_map.vec4Map.find("LocalRotation");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_positionCompEntity.localRotation = glm::qua<float>(this_vec4.w, this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }
    // "LocalTranslation", localTranslation.xyz = vec4.xyz  (optional)
    {
        auto search = in_map.vec4Map.find("LocalTranslation");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_positionCompEntity.localTranslation = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }
    // "GlobalTranslation", globalTranslation.xyz = vec4.xyz (optional)
    {
        auto search = in_map.vec4Map.find("LocalTranslation");
        if(search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_positionCompEntity.localTranslation = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }
    
    return this_positionCompEntity;
}

void PositionCompEntity::LocalScale(const glm::vec3 in_scale)
{
    localScale *= in_scale;
}

void PositionCompEntity::LocalRotate(const glm::qua<float> in_rotation)
{
    localRotation = in_rotation * localRotation;
}

void PositionCompEntity::LocalTranslate(const glm::vec3 in_translate)
{
    localTranslation += in_translate;
}

void PositionCompEntity::GlobalTranslate(const glm::vec3 in_translate)
{
    globalTranslation += in_translate;
}

glm::mat4x4 PositionCompEntity::GetGlobalMatrix(const glm::mat4x4 parent_global_matrix)
{
    glm::mat4 S_matrix = glm::scale(glm::mat4(1.0f), localScale);
    glm::mat4 R_matrix = glm::toMat4(localRotation);
    glm::mat4 T_matrix = glm::translate(glm::mat4(1.0f), localTranslation);
    glm::mat4 gT_matrix = glm::translate(glm::mat4(1.0f), globalTranslation);
    glm::mat4 gTTRS_matrix_in_global_space = gT_matrix * parent_global_matrix * T_matrix * R_matrix * S_matrix;

    return gTTRS_matrix_in_global_space;
}

void PositionCompEntity::Init()
{
}
