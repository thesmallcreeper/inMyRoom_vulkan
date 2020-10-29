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

EarlyNodeGlobalMatrixCompEntity EarlyNodeGlobalMatrixCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
{
    EarlyNodeGlobalMatrixCompEntity this_earlyNodeGlobalMatrixCompEntity(in_entity);

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

void EarlyNodeGlobalMatrixCompEntity::Update(EntitiesHandler* const entities_handler_ptr,
                                             NodeDataComp* const nodeDataComp_ptr,
                                             EarlyNodeGlobalMatrixComp* const earlyNodeGlobalMatrixComp_ptr)
{
    Entity parent_entity = entities_handler_ptr->GetParentOfEntity(thisEntity);
    NodeDataCompEntity& this_node_data = nodeDataComp_ptr->GetComponentEntity(thisEntity);

    if (parent_entity != 0)
    {
        EarlyNodeGlobalMatrixCompEntity& parent_nodeGlobalMatrix_componentEntity = earlyNodeGlobalMatrixComp_ptr->GetComponentEntity(parent_entity);
        globalMatrix = this_node_data.GetGlobalMatrix(parent_nodeGlobalMatrix_componentEntity.globalMatrix);
    }
    else
    {
        globalMatrix = this_node_data.GetGlobalMatrix(glm::mat4(1.f));
    }
}

#endif