#include "ECS/GeneralComponents/AnimationActorComp.h"
#include "ECS/GeneralComponents/PositionComp.h"

#include "ECS/ECSwrapper.h"

AnimationActorComp::AnimationActorComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentSparseBaseClass<AnimationActorCompEntity>(static_cast<componentID>(componentIDenum::AnimationActor), "AnimationActor", in_ecs_wrapper_ptr)
{
    AnimationActorCompEntity::animationActorComp_ptr = this;
}

AnimationActorComp::~AnimationActorComp()
{
    AnimationActorCompEntity::animationActorComp_ptr = nullptr;
}

void AnimationActorComp::Update() //ComponentSparseBaseClass
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::Position);
    PositionComp* const positionComp_ptr = static_cast<PositionComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    std::chrono::duration<float> delta_time = ecsWrapper_ptr->GetUpdateDeltaTime();

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].Update(positionComp_ptr, delta_time);
}