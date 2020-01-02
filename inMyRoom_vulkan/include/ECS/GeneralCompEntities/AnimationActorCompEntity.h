#pragma once

#include "ECS/ECStypes.h"

#include <chrono>
#include <unordered_map>
#include <string>

#include "glm/vec3.hpp"
#include "glm/gtx/quaternion.hpp"

struct AnimationData
{
    std::map<float, glm::vec3> timeToScaleKey_map;
    InterpolationType timeToScale_interpolation = InterpolationType::Linear;
    std::map<float, glm::qua<float>> timeToRotationKey_map;
    InterpolationType timeToRotation_interpolation = InterpolationType::Linear;
    std::map<float, glm::vec3> timeToTranslationKey_map;
    InterpolationType timeToTranslation_interpolation = InterpolationType::Linear;
};

#ifndef GAME_DLL
    class AnimationActorComp;
#endif


    // TODO cubic interpolation
class AnimationActorCompEntity
{
#ifndef GAME_DLL
public:
    AnimationActorCompEntity(const Entity this_entity);
    ~AnimationActorCompEntity();

    static AnimationActorCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - AnimationActorCompEntity
        "Animation_X",                              animations_umap[X]                  = string                (name/declaration of the animation)
        "ANIMATION-NAME_PATH_key_X_interpolation"   ANIMATION-NAME_input[X].PATH.intep  = int                   InterpolationType     
        "ANIMATION-NAME_PATH_key_X_time"            ANIMATION-NAME_input[X].PATH.time   = float                 (time of the key)
        "ANIMATION-NAME_PATH_key_X_data"            ANIMATION-NAME_input[X].PATH.data   = vec4.????             (path_data)
    */
    static AnimationActorCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();
    void Update(class PositionComp* const positionComp_ptr, const std::chrono::duration<float> deltaTime);

private: // help functions
    float GetAnimationTimeLength(const AnimationData& animation_data);

    glm::vec3 LinearInterpolationVec3(float animation_time, 
                                      std::pair<float, glm::vec3> timeAndTranslation_lower_pair,
                                      std::pair<float, glm::vec3> timeAndTranslation_upper_pair);
    glm::qua<float> LinearInterpolationQua(float animation_time, 
                                           std::pair<float, glm::qua<float>> timeAndTranslation_lower_pair,
                                           std::pair<float, glm::qua<float>> timeAndTranslation_upper_pair);

private: // static variable
    friend class AnimationActorComp;
    static AnimationActorComp* animationActorComp_ptr;
#endif

public: // public functions
    void StartAnimation(class ComponentBaseClass* const positionComp_bptr, std::string animation_name, bool should_loop, float time_offset);
    void FreezeAnimation();
    void UnfreezeAnimation();

public: // data
    std::unordered_map<std::string, size_t> animationNameToAnimationIndex_umap;
    std::vector<AnimationData> animationsData;

    bool currentAnimationFreezed = false;
    bool currentAnimationShouldLoop = false;
    size_t currentAnimation_index = -1;
    float currentAnimation_time = 0.f;

    glm::vec3 translationT0;
    glm::qua<float> rotationT0;
    glm::vec3 scaleT0;

    Entity thisEntity;

};