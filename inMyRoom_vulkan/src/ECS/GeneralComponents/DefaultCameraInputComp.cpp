#include "ECS/GeneralComponents/DefaultCameraInputComp.h"
#include "ECS/GeneralComponents/CameraComp.h"

#include "ECS/ECSwrapper.h"

DefaultCameraInputComp::DefaultCameraInputComp(ECSwrapper* const in_ecs_wrapper_ptr, float default_speed)
    :ComponentSparseBaseClass<DefaultCameraInputCompEntity>(static_cast<componentID>(componentIDenum::DefaultCameraInput), "DefaultCameraInput", in_ecs_wrapper_ptr),
     default_speed(default_speed)
{
    DefaultCameraInputCompEntity::defaultCameraInputComp_ptr= this;
}

DefaultCameraInputComp::~DefaultCameraInputComp()
{
    DefaultCameraInputCompEntity::defaultCameraInputComp_ptr = nullptr;
}

std::vector<std::pair<std::string, MapType>> DefaultCameraInputComp::GetComponentInitMapFields()
{
    return DefaultCameraInputCompEntity::GetComponentInitMapFields();
}

void DefaultCameraInputComp::Update()
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));
    for (DefaultCameraInputCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.Update(cameraComp_ptr, duration);

    lastSnapTimePoint = next_snap_timePoint;
}

void DefaultCameraInputComp::AsyncInput(InputType input_type, void* struct_data)
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));
    for (DefaultCameraInputCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.AsyncInput(input_type, struct_data, duration);

    lastSnapTimePoint = next_snap_timePoint;
}
