#pragma once

#include "ECS/GeneralCompEntities/AnimationComposerCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"



class AnimationComposerComp final
    :public ComponentDataClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer", sparse_set>
{
public:
    AnimationComposerComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~AnimationComposerComp() override;

    void Update() override;
};