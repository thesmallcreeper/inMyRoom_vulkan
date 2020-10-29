#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

AnimationComposerCompEntity::AnimationComposerCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<AnimationComposerComp>(this_entity)
{
    ECSwrapper_ptr = GetComponentPtr()->GetECSwrapper();
}

AnimationComposerCompEntity AnimationComposerCompEntity::GetEmpty()
{
    AnimationComposerCompEntity this_animationComposerCompEntity(0);

    return this_animationComposerCompEntity;
}

AnimationComposerCompEntity AnimationComposerCompEntity::CreateComponentEntityByMap(const Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map)
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

            Entity this_joint_relative_entity = GetComponentPtr()->GetECSwrapper()
                                                                 ->GetRelativeEntityOffset(entity_name,
                                                                                           this_relative_node_name);

            this_animationComposerCompEntity.actorRelativeEntities.emplace_back(this_joint_relative_entity);

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

void AnimationComposerCompEntity::Update(NodeDataComp* const positionComp_ptr,
                                         AnimationActorComp* const animationActorComp_ptr)
{
    if (shouldAutoplay)
    {
        StartAnimation(positionComp_ptr, animationActorComp_ptr, true, 0.f);
        shouldAutoplay = false;
    }
}
#endif

void AnimationComposerCompEntity::StartAnimation(NodeDataComp* const positionComp_ptr,
                                                 AnimationActorComp* const animationActorComp_ptr,
                                                 bool should_loop, float time_offset)
{
    for (Entity this_actor_entity : actorRelativeEntities)
    {
        AnimationActorCompEntity& this_actor_ptr = animationActorComp_ptr->GetComponentEntity(this_actor_entity + thisEntity);
        this_actor_ptr.StartAnimation(positionComp_ptr, animationName, should_loop, time_offset);
    }
}

void AnimationComposerCompEntity::FreezeAnimation(AnimationActorComp* const animationActorComp_ptr)
{
    for (Entity this_actor_entity : actorRelativeEntities)
    {
        AnimationActorCompEntity& this_actor = animationActorComp_ptr->GetComponentEntity(this_actor_entity + thisEntity);
        this_actor.FreezeAnimation();
    }
}

void AnimationComposerCompEntity::UnfreezeAnimation(AnimationActorComp* const animationActorComp_ptr)
{
    for (Entity this_actor_entity : actorRelativeEntities)
    {
        AnimationActorCompEntity& this_actor = animationActorComp_ptr->GetComponentEntity(this_actor_entity + thisEntity);
        this_actor.UnfreezeAnimation();
    }
}
