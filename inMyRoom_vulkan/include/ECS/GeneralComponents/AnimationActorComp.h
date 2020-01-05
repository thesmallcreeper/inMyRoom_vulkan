#pragma once

#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/AnimationsDataOfNodes.h"


class AnimationActorComp
    :public ComponentSparseBaseClass<AnimationActorCompEntity>
{
public:
    AnimationActorComp(ECSwrapper* const in_ecs_wrapper_ptr, AnimationsDataOfNodes* in_animationsDataOfNodes_ptr);
    ~AnimationActorComp();

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

private:
    AnimationsDataOfNodes* animationsDataOfNodes_ptr;
};