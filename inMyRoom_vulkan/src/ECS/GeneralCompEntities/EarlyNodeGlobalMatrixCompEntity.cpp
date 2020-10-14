#include "ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

EarlyNodeGlobalMatrixCompEntity::EarlyNodeGlobalMatrixCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<EarlyNodeGlobalMatrixComp>(this_entity)
{
}

EarlyNodeGlobalMatrixCompEntity EarlyNodeGlobalMatrixCompEntity::GetEmpty()
{
    EarlyNodeGlobalMatrixCompEntity this_nodeGlobalMatrixCompEntity(0);

    return this_nodeGlobalMatrixCompEntity;
}

EarlyNodeGlobalMatrixCompEntity EarlyNodeGlobalMatrixCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
{
    EarlyNodeGlobalMatrixCompEntity this_earlyNodeGlobalMatrixCompEntity(in_entity);

    // "ParentEntity", localScale.xyz = vec4.xyz    (optional)
    {
        {
            auto search = in_map.entityMap.find("ParentEntity");
            if (search != in_map.entityMap.end())
            {
                Entity this_parent_entity = search->second;
                this_earlyNodeGlobalMatrixCompEntity.parentEntity = this_parent_entity;
            }
        }
        {
            auto search = in_map.stringMap.find("ParentName");
            if (search != in_map.stringMap.end())
            {
                Entity this_entity = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()->FindEntityByRelativeName(search->second, in_entity);
                this_earlyNodeGlobalMatrixCompEntity.parentEntity = this_entity;
            }
        }
    }
    // "MatrixColumn0", globalMatrix[0] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn0");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_earlyNodeGlobalMatrixCompEntity.globalMatrix[0] = this_vec4;
        }
    }
    // "MatrixColumn1", globalMatrix[0] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn1");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_earlyNodeGlobalMatrixCompEntity.globalMatrix[1] = this_vec4;
        }
    }
    // "MatrixColumn2", globalMatrix[2] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn2");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_earlyNodeGlobalMatrixCompEntity.globalMatrix[0] = this_vec4;
        }
    }
    // "MatrixColumn3", globalMatrix[3] = vec4      (optional)
    {
        auto search = in_map.vec4Map.find("MatrixColumn3");
        if (search != in_map.vec4Map.end())
        {
            glm::vec4 this_vec4 = search->second;
            this_earlyNodeGlobalMatrixCompEntity.globalMatrix[0] = this_vec4;
        }
    }

    return this_earlyNodeGlobalMatrixCompEntity;
}

std::vector<std::pair<std::string, MapType>> EarlyNodeGlobalMatrixCompEntity::GetComponentInitMapFields()
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

void EarlyNodeGlobalMatrixCompEntity::Update(const NodeDataCompEntity& this_nodeData,
                                             EarlyNodeGlobalMatrixComp* const earlyNodeGlobalMatrixComp_ptr)
{
    if (parentEntity != 0)
    {
        EarlyNodeGlobalMatrixCompEntity& parent_nodeGlobalMatrix_componentEntity = earlyNodeGlobalMatrixComp_ptr->GetComponentEntity(parentEntity);
        globalMatrix = this_nodeData.GetGlobalMatrix(parent_nodeGlobalMatrix_componentEntity.globalMatrix);
    }
    else
        globalMatrix = this_nodeData.GetGlobalMatrix(glm::mat4(1.f));

}

void EarlyNodeGlobalMatrixCompEntity::Init()
{
}

#endif