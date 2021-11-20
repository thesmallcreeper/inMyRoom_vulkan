#include "ECS/GeneralComponents/AnimationComposerComp.h"

#include "ECS/ECSwrapper.h"

AnimationComposerComp::AnimationComposerComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer", sparse_set>(in_ecs_wrapper_ptr)
{
}

AnimationComposerComp::~AnimationComposerComp()
{
}
