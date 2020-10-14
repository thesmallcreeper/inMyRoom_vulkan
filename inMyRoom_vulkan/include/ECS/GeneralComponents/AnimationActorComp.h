#pragma once

#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/AnimationsDataOfNodes.h"


class AnimationActorComp final
    :public ComponentSparseBaseClass<AnimationActorCompEntity, static_cast<componentID>(componentIDenum::AnimationActor), "AnimationActor">
{
public:
    AnimationActorComp(ECSwrapper* const in_ecs_wrapper_ptr, AnimationsDataOfNodes* in_animationsDataOfNodes_ptr);
    ~AnimationActorComp() override;

    void Update() override;
private:
    AnimationsDataOfNodes* animationsDataOfNodes_ptr;
};