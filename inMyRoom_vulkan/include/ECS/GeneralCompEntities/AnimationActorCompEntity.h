#pragma once

#include "ECS/CompEntityBase.h"

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"

#include <chrono>
#include <unordered_map>
#include <string>


#ifdef GAME_DLL
class AnimationActorCompEntity;
class AnimationActorComp :
    public ComponentBaseWrappedClass<AnimationActorCompEntity, static_cast<componentID>(componentIDenum::AnimationActor), "AnimationActor"> {};
#else
class AnimationActorComp;
#endif

// TODO cubic interpolation
class AnimationActorCompEntity :
    public CompEntityBase<AnimationActorComp>
{
#ifndef GAME_DLL
public:
    AnimationActorCompEntity(Entity this_entity);

    static AnimationActorCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - AnimationActorCompEntity
        "Animation_X",  animations_umap[X]  = string  (name/declaration of the animation)
    */
    static AnimationActorCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(NodeDataComp* positionComp_ptr,
                class AnimationsDataOfNodes* animationDataOfNodes_ptr,
                std::chrono::duration<float> deltaTime);

private: // help functions
    float GetAnimationTimeLength(const AnimationData& animation_data);

    glm::vec3 LinearInterpolationVec3(float animation_time, 
                                      std::pair<float, glm::vec3> timeAndTranslation_lower_pair,
                                      std::pair<float, glm::vec3> timeAndTranslation_upper_pair);
    glm::qua<float> LinearInterpolationQua(float animation_time, 
                                           std::pair<float, glm::qua<float>> timeAndTranslation_lower_pair,
                                           std::pair<float, glm::qua<float>> timeAndTranslation_upper_pair);

#endif

public: // public functions
    void StartAnimation(NodeDataComp* positionComp_ptr, std::string animation_name, bool should_loop, float time_offset);
    void FreezeAnimation();
    void UnfreezeAnimation();

public: // data
    std::unordered_map<std::string, size_t> animationNameToAnimationIndex_umap;

    bool currentAnimationFreezed = false;
    bool currentAnimationShouldLoop = false;
    size_t currentAnimation_index = -1;
    float currentAnimation_time = 0.f;

    glm::vec3 translationT0;
    glm::qua<float> rotationT0;
    glm::vec3 scaleT0;
};