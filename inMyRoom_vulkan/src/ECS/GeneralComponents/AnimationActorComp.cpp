#include "ECS/GeneralComponents/AnimationActorComp.h"

#include "ECS/ECSwrapper.h"

AnimationActorComp::AnimationActorComp(ECSwrapper* const in_ecs_wrapper_ptr, AnimationsDataOfNodes* in_animationsDataOfNodes_ptr)
    :ComponentSparseBaseClass<AnimationActorCompEntity, static_cast<componentID>(componentIDenum::AnimationActor), "AnimationActor">(in_ecs_wrapper_ptr),
     animationsDataOfNodes_ptr(in_animationsDataOfNodes_ptr)
{
}

AnimationActorComp::~AnimationActorComp()
{
}

void AnimationActorComp::Update() //ComponentSparseBaseClass
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const positionComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    std::chrono::duration<float> delta_time = ecsWrapper_ptr->GetUpdateDeltaTime();

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].Update(positionComp_ptr, animationsDataOfNodes_ptr, delta_time);
}