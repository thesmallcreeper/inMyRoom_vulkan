#include "Components/SnakePlayerComp.h"

#include "ECS/ECSwrapper.h" 


SnakePlayerComp::SnakePlayerComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<SnakePlayerCompEntity, static_cast<componentID>(componentIDenum::SnakePlayer), "SnakePlayer", sparse_set>(in_ecs_wrapper_ptr)
{
    lastSnapTimePoint = std::chrono::steady_clock::now();
}

SnakePlayerComp::~SnakePlayerComp()
{
}

void SnakePlayerComp::Update()
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> async_duration = next_snap_timePoint - previous_snap_timePoint;

    componentID nodeData_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const nodeDataComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(nodeData_componentID));

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));

    componentID animationActor_componentID = static_cast<componentID>(componentIDenum::AnimationActor);
    AnimationActorComp* const animationActorComp_ptr = static_cast<AnimationActorComp*>(ecsWrapper_ptr->GetComponentByID(animationActor_componentID));

    componentID animationComposer_componentID = static_cast<componentID>(componentIDenum::AnimationComposer);
    AnimationComposerComp* const animationComposerComp_ptr = static_cast<AnimationComposerComp*>(ecsWrapper_ptr->GetComponentByID(animationComposer_componentID));

    for (SnakePlayerCompEntity& this_componentEntity : componentEntities)
        this_componentEntity.Update(nodeDataComp_ptr,
                                    cameraComp_ptr,
                                    animationActorComp_ptr,
                                    animationComposerComp_ptr,
                                    ecsWrapper_ptr->GetUpdateDeltaTime(),
                                    async_duration);

    lastSnapTimePoint = next_snap_timePoint;
}

void SnakePlayerComp::AsyncInput(InputType input_type, void* struct_data)
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    for (SnakePlayerCompEntity& this_componentEntity : componentEntities)
        this_componentEntity.AsyncInput(input_type, struct_data, duration);

    lastSnapTimePoint = next_snap_timePoint;
}

void SnakePlayerComp::CollisionCallback(const std::vector<std::pair<Entity, CollisionCallbackData>>& callback_entity_data_pairs)
{
    componentID nodeData_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const nodeDataComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(nodeData_componentID));

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));

    componentID cameraDefaultInput_componentID = static_cast<componentID>(componentIDenum::CameraDefaultInput);
    CameraDefaultInputComp* const cameraDefaultInput_ptr = static_cast<CameraDefaultInputComp*>(ecsWrapper_ptr->GetComponentByID(cameraDefaultInput_componentID));

    componentID animationActor_componentID = static_cast<componentID>(componentIDenum::AnimationActor);
    AnimationActorComp* const animationActorComp_ptr = static_cast<AnimationActorComp*>(ecsWrapper_ptr->GetComponentByID(animationActor_componentID));

    componentID animationComposer_componentID = static_cast<componentID>(componentIDenum::AnimationComposer);
    AnimationComposerComp* const animationComposerComp_ptr = static_cast<AnimationComposerComp*>(ecsWrapper_ptr->GetComponentByID(animationComposer_componentID));

    for(const auto& this_entity_data_pair:callback_entity_data_pairs)
    {
        if(componentEntities.does_exist(this_entity_data_pair.first))
        {
            componentEntities[this_entity_data_pair.first].CollisionUpdate(nodeDataComp_ptr,
                                                                           cameraComp_ptr,
                                                                           cameraDefaultInput_ptr,
                                                                           animationActorComp_ptr,
                                                                           animationComposerComp_ptr,
                                                                           this_entity_data_pair.second);
        }
    }
}


