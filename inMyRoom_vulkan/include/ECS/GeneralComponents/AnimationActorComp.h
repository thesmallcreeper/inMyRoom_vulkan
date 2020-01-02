#pragma once

#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class AnimationActorComp
    :public ComponentSparseBaseClass<AnimationActorCompEntity>
{
public:
    AnimationActorComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~AnimationActorComp();

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};
};