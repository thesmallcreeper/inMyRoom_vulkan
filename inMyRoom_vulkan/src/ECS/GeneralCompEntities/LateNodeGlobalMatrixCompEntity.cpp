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

LateNodeGlobalMatrixCompEntity LateNodeGlobalMatrixCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
{
    LateNodeGlobalMatrixCompEntity this_lateNodeGlobalMatrixCompEntity(in_entity);

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

void LateNodeGlobalMatrixCompEntity::Update(EntitiesHandler* const entities_handler_ptr,
                                            NodeDataComp* const nodeDataComp_ptr,
                                            LateNodeGlobalMatrixComp* const lateNodeGlobalMatrixComp_ptr)
{
    Entity parent_entity = entities_handler_ptr->GetParentOfEntity(thisEntity);
    NodeDataCompEntity& this_node_data = nodeDataComp_ptr->GetComponentEntity(thisEntity);

    if (parent_entity != 0)
    {
        LateNodeGlobalMatrixCompEntity& parent_nodeGlobalMatrix_componentEntity = lateNodeGlobalMatrixComp_ptr->GetComponentEntity(parent_entity);
        globalMatrix = this_node_data.GetGlobalMatrix(parent_nodeGlobalMatrix_componentEntity.globalMatrix);
    }
    else
    {
        globalMatrix = this_node_data.GetGlobalMatrix(glm::mat4(1.f));
    }
}

#endif