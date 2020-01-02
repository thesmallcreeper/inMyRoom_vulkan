#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL
#include "ECS/GeneralComponents/AnimationComposerComp.h"
#include "ECS/GeneralComponents/AnimationActorComp.h"
#include "..\..\..\include\ECS\GeneralCompEntities\AnimationComposerCompEntity.h"

AnimationComposerComp* AnimationComposerCompEntity::animationComposerComp_ptr = nullptr;

AnimationComposerCompEntity::AnimationComposerCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

AnimationComposerCompEntity::~AnimationComposerCompEntity()
{
}

AnimationComposerCompEntity AnimationComposerCompEntity::GetEmpty()
{
    AnimationComposerCompEntity this_animationComposerCompEntity(0);

    return this_animationComposerCompEntity;
}

AnimationComposerCompEntity AnimationComposerCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map)
{
    AnimationComposerCompEntity this_animationComposerCompEntity(in_entity);

    // "AnimationName",          animationName            = string    
    {
        auto search = in_map.stringMap.find("AnimationName");
        assert(search != in_map.stringMap.end());

        this_animationComposerCompEntity.animationName = search->second;
    }

    // "NodesRelativeName_X",    nodesEntities[X]         = string    (name)
    {
        size_t index = 0;
        std::string map_search_string = "NodesRelativeName_" + std::to_string(index);

        while (in_map.stringMap.find(map_search_string) != in_map.stringMap.end())
        {
            auto search = in_map.stringMap.find(map_search_string);
            std::string this_relative_node_name = search->second;

            Entity this_joint_entity = animationComposerComp_ptr->GetECSwrapper()->GetEntitiesHandler()
                                                                 ->FindEntityByRelativeName(this_relative_node_name,
                                                                                            this_animationComposerCompEntity.thisEntity);

            this_animationComposerCompEntity.actorEntities.emplace_back(this_joint_entity);

            map_search_string = "NodesRelativeName_" + std::to_string(++index);
        }
    }

    // "ShouldAutoplay",         shouldAutoplay            = int

    {
        auto search = in_map.intMap.find("ShouldAutoplay");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_animationComposerCompEntity.shouldAutoplay = static_cast<bool>(this_int);
        }          
    }

    return this_animationComposerCompEntity;
}

std::vector<std::pair<std::string, MapType>> AnimationComposerCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    return_pair.emplace_back(std::make_pair("ShouldAutoplay", MapType::int_type));   // TODO add bool

    return return_pair;
}

void AnimationComposerCompEntity::Init()
{
    if (shouldAutoplay)
    {
        StartAnimation(true, 0.f);
    }
}
#endif

void AnimationComposerCompEntity::StartAnimation(bool should_loop, float time_offset)
{
    componentID animationActor_componentID = static_cast<componentID>(componentIDenum::AnimationActor);
    AnimationActorComp* const animationActorComp_ptr = static_cast<AnimationActorComp*>(animationComposerComp_ptr->GetECSwrapper()->GetComponentByID(animationActor_componentID));

    componentID position_componentID = static_cast<componentID>(componentIDenum::Position);
    ComponentBaseClass* const positionComp_bptr = animationComposerComp_ptr->GetECSwrapper()->GetComponentByID(animationActor_componentID);

    for (Entity this_actor_entity : actorEntities)
    {
        AnimationActorCompEntity* this_actor_ptr = reinterpret_cast<AnimationActorCompEntity*>(animationActorComp_ptr->GetComponentEntity(this_actor_entity));
        this_actor_ptr->StartAnimation(positionComp_bptr, animationName, should_loop, time_offset);
    }
}

void AnimationComposerCompEntity::FreezeAnimation()
{
    componentID animationActor_componentID = static_cast<componentID>(componentIDenum::AnimationActor);
    AnimationActorComp* const animationActorComp_ptr = static_cast<AnimationActorComp*>(animationComposerComp_ptr->GetECSwrapper()->GetComponentByID(animationActor_componentID));

    for (Entity this_actor_entity : actorEntities)
    {
        AnimationActorCompEntity* this_actor_ptr = reinterpret_cast<AnimationActorCompEntity*>(animationActorComp_ptr->GetComponentEntity(this_actor_entity));
        this_actor_ptr->FreezeAnimation();
    }
}

void AnimationComposerCompEntity::UnfreezeAnimation()
{
    componentID animationActor_componentID = static_cast<componentID>(componentIDenum::AnimationActor);
    AnimationActorComp* const animationActorComp_ptr = static_cast<AnimationActorComp*>(animationComposerComp_ptr->GetECSwrapper()->GetComponentByID(animationActor_componentID));

    for (Entity this_actor_entity : actorEntities)
    {
        AnimationActorCompEntity* this_actor_ptr = reinterpret_cast<AnimationActorCompEntity*>(animationActorComp_ptr->GetComponentEntity(this_actor_entity));
        this_actor_ptr->UnfreezeAnimation();
    }
}
