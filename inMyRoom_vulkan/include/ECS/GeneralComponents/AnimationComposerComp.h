#pragma once

#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class AnimationComposerComp
    :public ComponentSparseBaseClass<AnimationComposerCompEntity>
{
public:
    AnimationComposerComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~AnimationComposerComp();

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};
};