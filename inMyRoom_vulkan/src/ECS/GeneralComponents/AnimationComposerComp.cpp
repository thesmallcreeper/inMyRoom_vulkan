#include "ECS/GeneralComponents/AnimationComposerComp.h"

#include "ECS/ECSwrapper.h"

AnimationComposerComp::AnimationComposerComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentSparseBaseClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer">(in_ecs_wrapper_ptr)
{
}

AnimationComposerComp::~AnimationComposerComp()
{
}