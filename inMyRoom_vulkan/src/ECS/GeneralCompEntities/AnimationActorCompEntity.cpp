#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL
#include "ECS/GeneralComponents/AnimationActorComp.h"
#include "ECS/GeneralComponents/PositionComp.h"

#include <cmath>
#include <algorithm>

#include "glm/ext/quaternion_common.hpp"

AnimationActorComp* AnimationActorCompEntity::animationActorComp_ptr = nullptr;

AnimationActorCompEntity::AnimationActorCompEntity(const Entity this_entity)
    :thisEntity(this_entity)
{
}

AnimationActorCompEntity::~AnimationActorCompEntity()
{
}

AnimationActorCompEntity AnimationActorCompEntity::GetEmpty()
{
    AnimationActorCompEntity this_animationActorCompEntity(0);

    return this_animationActorCompEntity;
}

AnimationActorCompEntity AnimationActorCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap in_map)
{
    AnimationActorCompEntity this_animationActorCompEntity(in_entity);
    // "Animation_X",  animations_umap[X]  = string  (name/declaration of the animation)
    {
        size_t index = 0;
        std::string map_search_string = "Animation_" + std::to_string(index);

        while (in_map.stringMap.find(map_search_string) != in_map.stringMap.end())
        {
            auto search = in_map.stringMap.find(map_search_string);
            std::string this_animation_name = search->second;

            size_t this_animation_index = this_animationActorCompEntity.animationsData.size();
            this_animationActorCompEntity.animationNameToAnimationIndex_umap.emplace(this_animation_name, this_animation_index);

            {
                AnimationData this_animationData;
                
                {
                    size_t key_index = 0;

                    // using time as finder
                    std::string map_translation_time_search_string = this_animation_name + "_translation_key_" + std::to_string(key_index) + "_time";
                    while (in_map.floatMap.find(map_translation_time_search_string) != in_map.floatMap.end())
                    {
                        // "ANIMATION-NAME_translation_key_X_time" ANIMATION-NAME_input[X].PATH.time = float (time of the key)
                        auto time_search = in_map.floatMap.find(map_translation_time_search_string);
                        float this_key_time = time_search->second;

                        // "ANIMATION-NAME_translation_key_X_data" ANIMATION-NAME_input[X].PATH.data = vec4.???? (path_data)                     
                        std::string map_translation_data_search_string = this_animation_name + "_translation_key_" + std::to_string(key_index) + "_data";
                        auto data_search = in_map.vec4Map.find(map_translation_data_search_string);
                        assert(data_search != in_map.vec4Map.end());

                        glm::vec4 this_key_data = data_search->second;
                        glm::vec3 this_key_translation = glm::vec3(this_key_data.x, this_key_data.y, this_key_data.z);

                        this_animationData.timeToTranslationKey_map.emplace(this_key_time, this_key_translation);

                        map_translation_time_search_string = this_animation_name + "_translation_key_" + std::to_string(++key_index) + "_time";
                    }

                    // "ANIMATION-NAME_translation_interpolation"   ANIMATION - NAME_input[X].PATH.intep = int                   InterpolationType
                    std::string map_translation_interpolation_search_string = this_animation_name + "_translation_interpolation";
                    if (in_map.intMap.find(map_translation_interpolation_search_string) != in_map.intMap.end())
                    {
                        auto search = in_map.intMap.find(map_translation_interpolation_search_string);
                        this_animationData.timeToTranslation_interpolation = static_cast<InterpolationType>(search->second);
                    }

                }

                {
                    size_t key_index = 0;

                    // using time as finder
                    std::string map_rotation_time_search_string = this_animation_name + "_rotation_key_" + std::to_string(key_index) + "_time";
                    while (in_map.floatMap.find(map_rotation_time_search_string) != in_map.floatMap.end())
                    {
                        // "ANIMATION-NAME_rotation_key_X_time" ANIMATION-NAME_input[X].PATH.time = float (time of the key)
                        auto time_search = in_map.floatMap.find(map_rotation_time_search_string);
                        float this_key_time = time_search->second;

                        // "ANIMATION-NAME_rotation_key_X_data" ANIMATION-NAME_input[X].PATH.data = vec4.???? (path_data)                     
                        std::string map_rotation_data_search_string = this_animation_name + "_rotation_key_" + std::to_string(key_index) + "_data";
                        auto data_search = in_map.vec4Map.find(map_rotation_data_search_string);
                        assert(data_search != in_map.vec4Map.end());

                        glm::vec4 this_key_data = data_search->second;
                        glm::qua<float> this_key_rotation = glm::qua<float>(this_key_data.w, this_key_data.x, this_key_data.y, this_key_data.z);

                        this_animationData.timeToRotationKey_map.emplace(this_key_time, this_key_rotation);

                        map_rotation_time_search_string = this_animation_name + "_rotation_key_" + std::to_string(++key_index) + "_time";
                    }

                    // "ANIMATION-NAME_rotation_interpolation"   ANIMATION - NAME_input[X].PATH.intep = int                   InterpolationType
                    std::string map_rotation_interpolation_search_string = this_animation_name + "_rotation_interpolation";
                    if (in_map.intMap.find(map_rotation_interpolation_search_string) != in_map.intMap.end())
                    {
                        auto search = in_map.intMap.find(map_rotation_interpolation_search_string);
                        this_animationData.timeToRotation_interpolation = static_cast<InterpolationType>(search->second);
                    }

                }

                {
                    size_t key_index = 0;

                    // using time as finder
                    std::string map_scale_time_search_string = this_animation_name + "_scale_key_" + std::to_string(key_index) + "_time";
                    while (in_map.floatMap.find(map_scale_time_search_string) != in_map.floatMap.end())
                    {
                        // "ANIMATION-NAME_scale_key_X_time" ANIMATION-NAME_input[X].PATH.time = float (time of the key)
                        auto time_search = in_map.floatMap.find(map_scale_time_search_string);
                        float this_key_time = time_search->second;

                        // "ANIMATION-NAME_scale_key_X_data" ANIMATION-NAME_input[X].PATH.data = vec4.???? (path_data)                     
                        std::string map_scale_data_search_string = this_animation_name + "_scale_key_" + std::to_string(key_index) + "_data";
                        auto data_search = in_map.vec4Map.find(map_scale_data_search_string);
                        assert(data_search != in_map.vec4Map.end());

                        glm::vec4 this_key_data = data_search->second;
                        glm::vec3 this_key_rotation = glm::vec3(this_key_data.x, this_key_data.y, this_key_data.z);

                        this_animationData.timeToScaleKey_map.emplace(this_key_time, this_key_rotation);

                        map_scale_time_search_string = this_animation_name + "_scale_key_" + std::to_string(++key_index) + "_time";
                    }

                    // "ANIMATION-NAME_scale_interpolation"   ANIMATION - NAME_input[X].PATH.intep = int                   InterpolationType
                    std::string map_scale_interpolation_search_string = this_animation_name + "_scale_interpolation";
                    if (in_map.intMap.find(map_scale_interpolation_search_string) != in_map.intMap.end())
                    {
                        auto search = in_map.intMap.find(map_scale_interpolation_search_string);
                        this_animationData.timeToScale_interpolation = static_cast<InterpolationType>(search->second);
                    }
                }

                this_animationActorCompEntity.animationsData.emplace_back(this_animationData);
            }

            map_search_string = "Animation_" + std::to_string(++index);
        }
    }

    return this_animationActorCompEntity;
}

std::vector<std::pair<std::string, MapType>> AnimationActorCompEntity::GetComponentInitMapFields()
{
    std::vector<std::pair<std::string, MapType>> return_pair;
    // NULL

    return return_pair;
}

void AnimationActorCompEntity::Init()
{
}

void AnimationActorCompEntity::Update(PositionComp* const positionComp_ptr, const std::chrono::duration<float> deltaTime)
{
    PositionCompEntity* current_position_componentEntity = reinterpret_cast<PositionCompEntity*>(positionComp_ptr->GetComponentEntity(thisEntity));

    if (!currentAnimationFreezed && currentAnimation_index != -1)
    {
        currentAnimation_time += deltaTime.count();
        AnimationData& animation_data = animationsData[currentAnimation_index];

        float animation_length = GetAnimationTimeLength(animation_data);
        if (currentAnimation_time <= animation_length || currentAnimation_time > animation_length && currentAnimationShouldLoop)
        {
            currentAnimation_time = std::fmodf(currentAnimation_time, animation_length);

            if (animation_data.timeToTranslationKey_map.size())
            {
                std::pair<float, glm::vec3> timeAndTranslation_lower_pair;
                std::pair<float, glm::vec3> timeAndTranslation_upper_pair;

                auto search_not_less_key = animation_data.timeToTranslationKey_map.lower_bound(currentAnimation_time);

                if (search_not_less_key == animation_data.timeToTranslationKey_map.begin())
                {
                    timeAndTranslation_lower_pair = std::make_pair(0.f, translationT0);
                    timeAndTranslation_upper_pair = *search_not_less_key;
                }
                else if (search_not_less_key != animation_data.timeToTranslationKey_map.end())
                {
                    timeAndTranslation_lower_pair = *(--search_not_less_key);
                    timeAndTranslation_upper_pair = *(++search_not_less_key);
                }
                else if (search_not_less_key == animation_data.timeToTranslationKey_map.end())
                {
                    timeAndTranslation_lower_pair = *search_not_less_key;
                    timeAndTranslation_upper_pair = *search_not_less_key;
                }

                current_position_componentEntity->localTranslation = LinearInterpolationVec3(currentAnimation_time,
                                                                                             timeAndTranslation_lower_pair,
                                                                                             timeAndTranslation_upper_pair);
            }

            if (animation_data.timeToRotationKey_map.size())
            {
                std::pair<float, glm::qua<float>> timeAndRotation_lower_pair;
                std::pair<float, glm::qua<float>> timeAndRotation_upper_pair;

                auto search_not_less_key = animation_data.timeToRotationKey_map.lower_bound(currentAnimation_time);

                if (search_not_less_key == animation_data.timeToRotationKey_map.begin())
                {
                    timeAndRotation_lower_pair = std::make_pair(0.f, rotationT0);
                    timeAndRotation_upper_pair = *search_not_less_key;
                }
                else if (search_not_less_key != animation_data.timeToRotationKey_map.end())
                {
                    timeAndRotation_lower_pair = *(--search_not_less_key);
                    timeAndRotation_upper_pair = *(++search_not_less_key);
                }
                else if (search_not_less_key == animation_data.timeToRotationKey_map.end())
                {
                    timeAndRotation_lower_pair = *search_not_less_key;
                    timeAndRotation_upper_pair = *search_not_less_key;
                }

                current_position_componentEntity->localRotation = LinearInterpolationQua(currentAnimation_time,
                                                                                         timeAndRotation_lower_pair,
                                                                                         timeAndRotation_upper_pair);
            }

            if (animation_data.timeToScaleKey_map.size())
            {
                std::pair<float, glm::vec3> timeAndScale_lower_pair;
                std::pair<float, glm::vec3> timeAndScale_upper_pair;

                auto search_not_less_key = animation_data.timeToScaleKey_map.lower_bound(currentAnimation_time);

                if (search_not_less_key == animation_data.timeToScaleKey_map.begin())
                {
                    timeAndScale_lower_pair = std::make_pair(0.f, scaleT0);
                    timeAndScale_upper_pair = *search_not_less_key;
                }
                else if (search_not_less_key != animation_data.timeToScaleKey_map.end())
                {
                    timeAndScale_lower_pair = *(--search_not_less_key);
                    timeAndScale_upper_pair = *(++search_not_less_key);
                }
                else if (search_not_less_key == animation_data.timeToScaleKey_map.end())
                {
                    timeAndScale_lower_pair = *search_not_less_key;
                    timeAndScale_upper_pair = *search_not_less_key;
                }

                current_position_componentEntity->localScale = LinearInterpolationVec3(currentAnimation_time,
                                                                                       timeAndScale_lower_pair, 
                                                                                       timeAndScale_upper_pair);
            }
        }
        else
        {
            currentAnimationFreezed = false;
            currentAnimationShouldLoop = false;
            currentAnimation_index = -1;
            currentAnimation_time = 0.f;
        }
    }
}

float AnimationActorCompEntity::GetAnimationTimeLength(const AnimationData& animation_data)
{
    float return_timeLength = 0.f;
    if (animation_data.timeToTranslationKey_map.size())
        return_timeLength = std::max(return_timeLength, animation_data.timeToTranslationKey_map.rbegin()->first);
    if (animation_data.timeToRotationKey_map.size())
        return_timeLength = std::max(return_timeLength, animation_data.timeToRotationKey_map.rbegin()->first);
    if (animation_data.timeToScaleKey_map.size())
        return_timeLength = std::max(return_timeLength, animation_data.timeToScaleKey_map.rbegin()->first);

    return return_timeLength;
}

glm::vec3 AnimationActorCompEntity::LinearInterpolationVec3(float animation_time, 
                                                            std::pair<float, glm::vec3> timeAndTranslation_lower_pair,
                                                            std::pair<float, glm::vec3> timeAndTranslation_upper_pair)
{
    float delta_time = timeAndTranslation_upper_pair.first - timeAndTranslation_lower_pair.first;

    float lower_gravity = std::clamp(1.f - (animation_time - timeAndTranslation_lower_pair.first) / delta_time, 0.f, 1.f);
    float upper_gravity = 1.f - lower_gravity;

    glm::vec3 return_interpolated_vec3 = lower_gravity * timeAndTranslation_lower_pair.second + upper_gravity * timeAndTranslation_upper_pair.second;
    return return_interpolated_vec3;
}

glm::qua<float> AnimationActorCompEntity::LinearInterpolationQua(float animation_time,
                                                                 std::pair<float, glm::qua<float>> timeAndTranslation_lower_pair,
                                                                 std::pair<float, glm::qua<float>> timeAndTranslation_upper_pair)
{
    float delta_time = timeAndTranslation_upper_pair.first - timeAndTranslation_lower_pair.first;

    float lower_gravity = std::clamp(1.f - (animation_time - timeAndTranslation_lower_pair.first) / delta_time, 0.f, 1.f);
    float upper_gravity = 1.f - lower_gravity;

    glm::qua<float> return_interpolated_quat = glm::mix(timeAndTranslation_lower_pair.second,
                                                        timeAndTranslation_upper_pair.second,
                                                        upper_gravity);
    return return_interpolated_quat;
}

#endif

void AnimationActorCompEntity::StartAnimation(ComponentBaseClass* const positionComp_bptr, std::string animation_name, bool should_loop, float time_offset)
{
    auto search = animationNameToAnimationIndex_umap.find(animation_name);
    assert(search != animationNameToAnimationIndex_umap.end());

    currentAnimation_index = search->second;
    currentAnimation_time = time_offset;
    currentAnimationShouldLoop = should_loop;
    currentAnimationFreezed = false;

    PositionCompEntity* current_position_componentEntity = reinterpret_cast<PositionCompEntity*>(positionComp_bptr->GetComponentEntity(thisEntity));
    translationT0 = current_position_componentEntity->localTranslation;
    rotationT0 = current_position_componentEntity->localRotation;
    scaleT0 = current_position_componentEntity->localScale;
}

void AnimationActorCompEntity::FreezeAnimation()
{
    currentAnimationFreezed = true;
}

void AnimationActorCompEntity::UnfreezeAnimation()
{
    currentAnimationFreezed = false;
}