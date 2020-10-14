#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL
#include <cmath>
#include <algorithm>

#include "Graphics/Meshes/AnimationsDataOfNodes.h"

#include "glm/ext/quaternion_common.hpp"

AnimationActorCompEntity::AnimationActorCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<AnimationActorComp>(this_entity)
{
}

AnimationActorCompEntity AnimationActorCompEntity::GetEmpty()
{
    AnimationActorCompEntity this_animationActorCompEntity(0);

    return this_animationActorCompEntity;
}

AnimationActorCompEntity AnimationActorCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
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

            {
                auto search = in_map.intMap.find(this_animation_name + "_animationIndex");
                assert(search != in_map.intMap.end());

                size_t this_animation_index = static_cast<size_t>(search->second);
                this_animationActorCompEntity.animationNameToAnimationIndex_umap.emplace(this_animation_name, this_animation_index);
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

void AnimationActorCompEntity::Update(NodeDataComp* const positionComp_ptr,
                                      AnimationsDataOfNodes* const animationDataOfNodes_ptr, 
                                      const std::chrono::duration<float> deltaTime)
{
    NodeDataCompEntity& current_position_componentEntity = positionComp_ptr->GetComponentEntity(thisEntity);

    if (!currentAnimationFreezed && currentAnimation_index != -1)
    {
        currentAnimation_time += deltaTime.count();
        const AnimationData& animation_data = animationDataOfNodes_ptr->GetAnimationDataRef(currentAnimation_index);

        float animation_length = GetAnimationTimeLength(animation_data);
        if (currentAnimation_time <= animation_length || currentAnimation_time > animation_length && currentAnimationShouldLoop)
        {
            if (currentAnimation_time > animation_length)
            {
                currentAnimation_time = std::fmodf(currentAnimation_time, animation_length);
                translationT0 = current_position_componentEntity.localTranslation;
                rotationT0 = current_position_componentEntity.localRotation;
                scaleT0 = current_position_componentEntity.localScale;
            }
            

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
                    timeAndTranslation_lower_pair = *(--search_not_less_key);
                    timeAndTranslation_upper_pair = std::make_pair(0.f, translationT0);
                }

                current_position_componentEntity.localTranslation = LinearInterpolationVec3(currentAnimation_time,
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
                    timeAndRotation_lower_pair = *(--search_not_less_key);
                    timeAndRotation_upper_pair = std::make_pair(0.f, rotationT0);
                }

                current_position_componentEntity.localRotation = LinearInterpolationQua(currentAnimation_time,
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
                    timeAndScale_lower_pair = *(--search_not_less_key);
                    timeAndScale_upper_pair = std::make_pair(0.f, scaleT0);
                }

                current_position_componentEntity.localScale = LinearInterpolationVec3(currentAnimation_time,
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
    
    // TODO: wait debugged glm::mix
    glm::mat4x4 lower_transform = glm::mat4_cast(timeAndTranslation_lower_pair.second);
    glm::mat4x4 upper_transform = glm::mat4_cast(timeAndTranslation_upper_pair.second);
                                                         
    glm::mat4x4 interpolated_transform = lower_gravity * lower_transform  + upper_gravity * upper_transform;
    
    return glm::quat_cast<float>(interpolated_transform);
}

#endif

void AnimationActorCompEntity::StartAnimation(NodeDataComp* const positionComp_ptr, std::string animation_name, bool should_loop, float time_offset)
{
    auto search = animationNameToAnimationIndex_umap.find(animation_name);
    assert(search != animationNameToAnimationIndex_umap.end());

    currentAnimation_index = search->second;
    currentAnimation_time = time_offset;
    currentAnimationShouldLoop = should_loop;
    currentAnimationFreezed = false;

    NodeDataCompEntity& current_position_componentEntity = positionComp_ptr->GetComponentEntity(thisEntity);
    translationT0 = current_position_componentEntity.localTranslation;
    rotationT0 = current_position_componentEntity.localRotation;
    scaleT0 = current_position_componentEntity.localScale;
}

void AnimationActorCompEntity::FreezeAnimation()
{
    currentAnimationFreezed = true;
}

void AnimationActorCompEntity::UnfreezeAnimation()
{
    currentAnimationFreezed = false;
}