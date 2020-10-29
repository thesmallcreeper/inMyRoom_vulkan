#include "ECS/GeneralComponents/AnimationComposerComp.h"

#include "ECS/ECSwrapper.h"

AnimationComposerComp::AnimationComposerComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<AnimationComposerCompEntity, static_cast<componentID>(componentIDenum::AnimationComposer), "AnimationComposer", sparse_set>(in_ecs_wrapper_ptr)
{
}

AnimationComposerComp::~AnimationComposerComp()
{
}

void AnimationComposerComp::Update()
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const positionComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    componentID animationActor_componentID = static_cast<componentID>(componentIDenum::AnimationActor);
    AnimationActorComp* const animationActorComp_ptr = static_cast<AnimationActorComp*>(ecsWrapper_ptr->GetComponentByID(animationActor_componentID));

    for(auto& this_comp_entity: componentEntities)
    {
        this_comp_entity.Update(positionComp_ptr, animationActorComp_ptr);
    }
}