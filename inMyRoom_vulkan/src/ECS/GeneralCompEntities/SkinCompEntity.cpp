#include "ECS/GeneralCompEntities/SkinCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

SkinCompEntity::SkinCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<SkinComp>(this_entity)
{
}

SkinCompEntity SkinCompEntity::GetEmpty()
{
    SkinCompEntity this_skinCompEntity(0);

    return this_skinCompEntity;
}

SkinCompEntity SkinCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
{
    SkinCompEntity this_skinCompEntity(in_entity);

    // "InverseBindMatricesOffset", inverseBindMatricesOffset = int
    {
        auto search = in_map.intMap.find("InverseBindMatricesOffset");
        assert(search != in_map.intMap.end());

        int this_int = search->second;
        this_skinCompEntity.inverseBindMatricesOffset = static_cast<uint32_t>(this_int);
    }

    // "JointRelativeName_X", jointEntities[X] = string(name)
    {
        size_t index = 0;
        std::string map_search_string = "JointRelativeName_" + std::to_string(index);

        while (in_map.stringMap.find(map_search_string) != in_map.stringMap.end())
        {
            auto search = in_map.stringMap.find(map_search_string);
            std::string this_relative_node_name = search->second;

            Entity this_joint_entity = GetComponentPtr()->GetECSwrapper()->GetEntitiesHandler()
                                                        ->FindEntityByRelativeName(this_relative_node_name,
                                                                                   this_skinCompEntity.thisEntity);

            this_skinCompEntity.jointEntities.emplace_back(this_joint_entity);

            map_search_string = "JointRelativeName_" + std::to_string(++index);
        }
    }

    return this_skinCompEntity;
}

std::vector<std::pair<std::string, MapType>> SkinCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    // NULL

    return return_pair;
}

void SkinCompEntity::Init()
{
}

void SkinCompEntity::Update(LateNodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr, SkinsOfMeshes* skinsOfMeshes_ptr)
{
    lastNodesMatricesOffset = static_cast<uint32_t>(skinsOfMeshes_ptr->GetNodesRecordSize());

    for (const Entity this_jointEntity : jointEntities)
    {
        LateNodeGlobalMatrixCompEntity& this_nodeGlobalMatrixCompEntity_ptr = nodeGlobalMatrixComp_ptr->GetComponentEntity(this_jointEntity);
        glm::mat4 joint_global_matrix = this_nodeGlobalMatrixCompEntity_ptr.globalMatrix;
            
        skinsOfMeshes_ptr->AddNodeMatrix(joint_global_matrix);
    }
}

#endif