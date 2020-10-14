#pragma once

#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class AnimationComposerComp final
    :public ComponentSparseBaseClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer">
{
public:
    AnimationComposerComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~AnimationComposerComp() override;

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};
};