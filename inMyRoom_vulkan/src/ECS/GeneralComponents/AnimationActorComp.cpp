#include "ECS/GeneralComponents/AnimationActorComp.h"

#include "ECS/ECSwrapper.h"

AnimationActorComp::AnimationActorComp(ECSwrapper* const in_ecs_wrapper_ptr, AnimationsDataOfNodes* in_animationsDataOfNodes_ptr)
    :ComponentDataClass<AnimationActorCompEntity, static_cast<componentID>(componentIDenum::AnimationActor), "AnimationActor", sparse_set>(in_ecs_wrapper_ptr),
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

    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.Update(positionComp_ptr, animationsDataOfNodes_ptr, delta_time);
    }
}