#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class AnimationComposerComp;

#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"

class AnimationComposerCompEntity :
    public CompEntityBaseWrappedClass<AnimationComposerComp>
{
#ifndef GAME_DLL
public:
    AnimationComposerCompEntity(Entity this_entity);

    static AnimationComposerCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SkinCompEntity
        "AnimationName",                 animationName                 = string    
        "NodesRelativeName_X",           nodesEntities[X]              = string    (name)
        "ShouldAutoplay",                shouldAutoplay                = int
    */
    static AnimationComposerCompEntity CreateComponentEntityByMap(Entity in_entity, std::string entity_name, const CompEntityInitMap& in_map);

    void Update(NodeDataComp* positionComp_ptr,
                AnimationActorComp* animationActorComp_ptr);

#endif
public: // public functions
    void StartAnimation(NodeDataComp* positionComp_ptr,
                        AnimationActorComp* animationActorComp_ptr,
                        bool should_loop, float time_offset = 0.f);
    void FreezeAnimation(AnimationActorComp* animationActorComp_ptr);
    void UnfreezeAnimation(AnimationActorComp* animationActorComp_ptr);

public: // data
    std::string animationName;
    std::vector<Entity> actorRelativeEntities;

    bool shouldAutoplay = false;

    class ECSwrapper* ECSwrapper_ptr;
};

#ifdef GAME_DLL
class AnimationComposerComp :
    public ComponentBaseWrappedClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer", sparse_set> {};
#else
#include "ECS/GeneralComponents/AnimationComposerComp.h"
#endif