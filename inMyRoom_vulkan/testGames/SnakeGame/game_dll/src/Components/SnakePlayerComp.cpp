#include "Components/SnakePlayerComp.h"

#include "ECS/ECSwrapper.h" 


SnakePlayerComp::SnakePlayerComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentSparseBaseClass<SnakePlayerCompEntity>(static_cast<componentID>(componentIDenum::SnakePlayer), "SnakePlayer", in_ecs_wrapper_ptr)
{
    SnakePlayerCompEntity::snakePlayerComp_ptr = this;
}

SnakePlayerComp::~SnakePlayerComp()
{
    SnakePlayerCompEntity::snakePlayerComp_ptr = nullptr;
}

void SnakePlayerComp::Update()
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    componentID nodeData_componentID = static_cast<componentID>(componentIDenum::NodeData);
    ComponentBaseClass* const nodeDataComp_bptr = ecsWrapper_ptr->GetComponentByID(nodeData_componentID);

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    ComponentBaseClass* const cameraComp_bptr = ecsWrapper_ptr->GetComponentByID(camera_componentID);

    componentID animationComposer_componentID = static_cast<componentID>(componentIDenum::AnimationComposer);
    ComponentBaseClass* const animationComposerComp_bptr = ecsWrapper_ptr->GetComponentByID(animationComposer_componentID);

    for (SnakePlayerCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.Update(nodeDataComp_bptr, cameraComp_bptr, animationComposerComp_bptr, duration);

    lastSnapTimePoint = next_snap_timePoint;
}

void SnakePlayerComp::AsyncInput(InputType input_type, void* struct_data)
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    for (SnakePlayerCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.AsyncInput(input_type, struct_data, duration);

    lastSnapTimePoint = next_snap_timePoint;
}


