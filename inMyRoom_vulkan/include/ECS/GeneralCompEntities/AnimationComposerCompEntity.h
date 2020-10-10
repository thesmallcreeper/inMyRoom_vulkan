#pragma once

#include "ECS/CompEntityBase.h"

#ifdef GAME_DLL
class AnimationComposerCompEntity;
class AnimationComposerComp :
    public ComponentBaseWrappedClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer"> {};
#else
class AnimationComposerComp;
#endif

class AnimationComposerCompEntity :
    public CompEntityBase<AnimationComposerComp>
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