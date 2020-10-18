#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

LateNodeGlobalMatrixCompEntity::LateNodeGlobalMatrixCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<LateNodeGlobalMatrixComp>(this_entity)
{
}

LateNodeGlobalMatrixCompEntity LateNodeGlobalMatrixCompEntity::GetEmpty()
{
    LateNodeGlobalMatrixCompEntity this_nodeGlobalMatrixCompEntity(0);

    return this_nodeGlobalMatrixCompEntity;
}

LateNodeGlobalMatrixCompEntity LateNodeGlobalMatrixCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
{
    LateNodeGlobalMatrixCompEntity this_lateNodeGlobalMatrixCompEntity(in_entity);

    // "ParentEntity", localScale.xyz = vec4.xyz    (optional)
    {
        {
            auto search = in_map.intMap.find("ParentEntity");
            if (search != in_map.intMap.end())
            {
                Entity this_parent_entity = static_cast<Entity>(search->second);
                this_lateNodeGlobalMatrixCompEntity.parentEntity = this_parent_entity;
            }
        }
        {
            auto search = in_map.stringMap.find("ParentName");
            if (search != in_map.stringMap.end())
            {
                Entity this_entity = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->FindEntityByRelativeName(search->second, in_entity);
                this_lateNodeGlobalMatrixCompEntity.parentEntity = this_entity;
            }
        }
    }
    // "MatrixColumn0", globalMatrix[0] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn0");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_lateNodeGlobalMatrixCompEntity.globalMatrix[0] = this_vec4;
        }
    }
    // "MatrixColumn1", globalMatrix[0] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn1");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_lateNodeGlobalMatrixCompEntity.globalMatrix[1] = this_vec4;
        }
    }
    // "MatrixColumn2", globalMatrix[2] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn2");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_lateNodeGlobalMatrixCompEntity.globalMatrix[0] = this_vec4;
        }
    }
    // "MatrixColumn3", globalMatrix[3] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn3");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_lateNodeGlobalMatrixCompEntity.globalMatrix[0] = this_vec4;
        }
    }

    return this_lateNodeGlobalMatrixCompEntity;
}

void LateNodeGlobalMatrixCompEntity::Update(const NodeDataCompEntity& this_nodeData,
                                            LateNodeGlobalMatrixComp* const lateNodeGlobalMatrixComp_ptr)
{
    if (parentEntity != 0)
    {
        LateNodeGlobalMatrixCompEntity& parent_nodeGlobalMatrix_componentEntity = lateNodeGlobalMatrixComp_ptr->GetComponentEntity(parentEntity);
        globalMatrix = this_nodeData.GetGlobalMatrix(parent_nodeGlobalMatrix_componentEntity.globalMatrix);
    }
    else
        globalMatrix = this_nodeData.GetGlobalMatrix(glm::mat4(1.f));

}

void LateNodeGlobalMatrixCompEntity::Init()
{
}

#endif