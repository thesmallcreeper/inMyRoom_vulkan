#pragma once

#include "ECS/ECStypes.h"

#ifndef GAME_DLL
class AnimationComposerComp;
#endif

class AnimationComposerCompEntity
{
#ifndef GAME_DLL
public:
    AnimationComposerCompEntity(const Entity this_entity);
    ~AnimationComposerCompEntity();

    static AnimationComposerCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - SkinCompEntity
        "AnimationName",                 animationName                 = string    
        "NodesRelativeName_X",           nodesEntities[X]              = string    (name)
        "ShouldAutoplay",                shouldAutoplay                = int
    */
    static AnimationComposerCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();

private: // static_variable
    friend class AnimationComposerComp;
    static AnimationComposerComp* animationComposerComp_ptr;

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
    Entity thisEntity;
};