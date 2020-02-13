#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/NodeDataComp.h"

LateNodeGlobalMatrixComp* LateNodeGlobalMatrixCompEntity::lateNodeGlobalMatrixComp_ptr = nullptr;

LateNodeGlobalMatrixCompEntity::LateNodeGlobalMatrixCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

LateNodeGlobalMatrixCompEntity::~LateNodeGlobalMatrixCompEntity()
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
            auto search = in_map.entityMap.find("ParentEntity");
            if (search != in_map.entityMap.end())
            {
                Entity this_parent_entity = search->second;
                this_lateNodeGlobalMatrixCompEntity.parentEntity = this_parent_entity;
            }
        }
        {
            auto search = in_map.stringMap.find("ParentName");
            if (search != in_map.stringMap.end())
            {
                Entity this_entity = lateNodeGlobalMatrixComp_ptr->GetECSwrapper()->GetEntitiesHandler()->FindEntityByRelativeName(search->second, in_entity);
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

std::vector<std::pair<std::string, MapType>> LateNodeGlobalMatrixCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    return_pair.emplace_back(std::make_pair("ParentEntity",     MapType::entity_type));
    return_pair.emplace_back(std::make_pair("ParentName",       MapType::string_type));
    return_pair.emplace_back(std::make_pair("MatrixColumn0",    MapType::vec4_type));
    return_pair.emplace_back(std::make_pair("MatrixColumn1",    MapType::vec4_type));
    return_pair.emplace_back(std::make_pair("MatrixColumn2",    MapType::vec4_type));
    return_pair.emplace_back(std::make_pair("MatrixColumn3",    MapType::vec4_type));

    return return_pair;
}

void LateNodeGlobalMatrixCompEntity::Update(NodeDataComp* const positionComp_ptr)
{
    NodeDataCompEntity* current_position_componentEntity = reinterpret_cast<NodeDataCompEntity*>(positionComp_ptr->GetComponentEntity(thisEntity));

    if (parentEntity != 0)
    {
        LateNodeGlobalMatrixCompEntity* parent_nodeGlobalMatrix_componentEntity = reinterpret_cast<LateNodeGlobalMatrixCompEntity*>(lateNodeGlobalMatrixComp_ptr->GetComponentEntity(parentEntity));
        globalMatrix = current_position_componentEntity->GetGlobalMatrix(parent_nodeGlobalMatrix_componentEntity->globalMatrix);
    }
    else
        globalMatrix = current_position_componentEntity->GetGlobalMatrix(glm::mat4(1.f));

}

void LateNodeGlobalMatrixCompEntity::Init()
{
}

#endif