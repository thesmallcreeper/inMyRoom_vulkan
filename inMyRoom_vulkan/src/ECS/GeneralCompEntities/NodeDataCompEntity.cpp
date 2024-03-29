#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

NodeDataCompEntity::NodeDataCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<NodeDataComp>(this_entity)
{
}

NodeDataCompEntity NodeDataCompEntity::GetEmpty()
{
    NodeDataCompEntity this_positionCompEntity(0);

    return this_positionCompEntity;
}

NodeDataCompEntity NodeDataCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
{
    NodeDataCompEntity this_positionCompEntity(in_entity);

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
            this_positionCompEntity.localRotation = glm::normalize(glm::qua<float>(this_vec4.w, this_vec4.x, this_vec4.y, this_vec4.z));
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
        auto search = in_map.vec4Map.find("GlobalTranslation");
        if(search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_positionCompEntity.globalTranslation = glm::vec3(this_vec4.x, this_vec4.y, this_vec4.z);
        }
    }
    
    return this_positionCompEntity;
}

glm::mat4x4 NodeDataCompEntity::GetGlobalMatrix(const glm::mat4x4 parent_global_matrix) const
{
    glm::mat4 S_matrix = glm::scale(glm::mat4(1.0f), localScale);
    glm::mat4 R_matrix = glm::toMat4(localRotation);
    glm::mat4 T_matrix = glm::translate(glm::mat4(1.0f), localTranslation);
    glm::mat4 gT_matrix = glm::translate(glm::mat4(1.0f), globalTranslation);
    glm::mat4 gTTRS_matrix_in_global_space = gT_matrix * parent_global_matrix * T_matrix * R_matrix * S_matrix;

    return gTTRS_matrix_in_global_space;
}

void NodeDataCompEntity::Update()
{
    localScale_old = localScale;
    localRotation_old = localRotation;
    localTranslation_old = localTranslation;
    globalTranslation_old = globalTranslation;
}

#endif

void NodeDataCompEntity::LocalScale(const glm::vec3 in_scale)
{
    localScale *= in_scale;
}

void NodeDataCompEntity::LocalRotate(const glm::qua<float> in_rotation)
{
    localRotation = in_rotation * localRotation;
}

void NodeDataCompEntity::LocalTranslate(const glm::vec3 in_translate)
{
    localTranslation += in_translate;
}

void NodeDataCompEntity::GlobalTranslate(const glm::vec3 in_translate)
{
    globalTranslation += in_translate;
}