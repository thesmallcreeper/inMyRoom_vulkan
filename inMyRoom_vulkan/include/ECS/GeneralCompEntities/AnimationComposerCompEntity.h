#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class AnimationComposerComp;
class AnimationComposerCompEntity;
#ifdef GAME_DLL
class AnimationComposerComp :
    public ComponentBaseWrappedClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer"> {};
#else
#include "ECS/GeneralComponents/AnimationComposerComp.h"
#endif

#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"

class AnimationComposerCompEntity :
    public CompEntityBaseWrappedClass<AnimationComposerComp>
{
#ifndef GAME_DLL
public:
    AnimationComposerCompEntity(const Entity this_entity);

    static AnimationComposerCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SkinCompEntity
        "AnimationName",                 animationName                 = string    
        "NodesRelativeName_X",           nodesEntities[X]              = string    (name)
        "ShouldAutoplay",                shouldAutoplay                = int
    */
    static AnimationComposerCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();

#endif
public: // public functions
    void StartAnimation(bool should_loop, float time_offset = 0.f);
    void FreezeAnimation();
    void UnfreezeAnimation();

public: // data
    std::string animationName;
    std::vector<Entity> actorEntities;

    bool shouldAutoplay = false;

    class ECSwrapper* ECSwrapper_ptr;
};