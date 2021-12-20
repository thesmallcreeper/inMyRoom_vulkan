#include "ECS/GeneralCompEntities/DynamicMeshCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

DynamicMeshCompEntity::DynamicMeshCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<DynamicMeshComp>(this_entity)
{
}

DynamicMeshCompEntity DynamicMeshCompEntity::GetEmpty()
{
    DynamicMeshCompEntity this_skinCompEntity(0);

    return this_skinCompEntity;
}

DynamicMeshCompEntity DynamicMeshCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
{
    DynamicMeshCompEntity this_skinCompEntity(in_entity);

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

            Entity this_joint_entity = GetComponentPtr()->GetECSwrapper()
                                                        ->GetRelativeEntityOffset(entity_name, 
                                                                                  this_relative_node_name);

            this_skinCompEntity.jointRelativeEntities.emplace_back(this_joint_entity);

            map_search_string = "JointRelativeName_" + std::to_string(++index);
        }
    }

    // "DefaultWeight_X", morphTargetsWeights[X] = float
    {
        size_t index = 0;
        std::string map_search_string = "DefaultWeight_" + std::to_string(index);

        while (in_map.floatMap.find(map_search_string) != in_map.floatMap.end())
        {
            auto search = in_map.floatMap.find(map_search_string);
            float this_weight = search->second;

            this_skinCompEntity.morphTargetsWeights.emplace_back(this_weight);

            map_search_string = "DefaultWeight_" + std::to_string(++index);
        }
    }

    return this_skinCompEntity;
}

#endif