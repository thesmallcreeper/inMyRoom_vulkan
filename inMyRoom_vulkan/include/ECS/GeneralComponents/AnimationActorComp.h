#pragma once

#include "ECS/GeneralCompEntities/AnimationActorCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/AnimationsDataOfNodes.h"


class AnimationActorComp final
    :public ComponentDataClass<AnimationActorCompEntity, static_cast<componentID>(componentIDenum::AnimationActor), "AnimationActor", sparse_set>
{
public:
    explicit AnimationActorComp(ECSwrapper* in_ecs_wrapper_ptr, AnimationsDataOfNodes* in_animationsDataOfNodes_ptr);
    ~AnimationActorComp() override;

    void Update() override;
private:
    AnimationsDataOfNodes* animationsDataOfNodes_ptr;
};