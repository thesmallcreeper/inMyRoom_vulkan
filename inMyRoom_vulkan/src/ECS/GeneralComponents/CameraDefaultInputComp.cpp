#include "ECS/GeneralComponents/CameraDefaultInputComp.h"
#include "ECS/GeneralComponents/CameraComp.h"

#include "ECS/ECSwrapper.h"

CameraDefaultInputComp::CameraDefaultInputComp(ECSwrapper* const in_ecs_wrapper_ptr, float default_speed)
    :ComponentSparseBaseClass<CameraDefaultInputCompEntity>(static_cast<componentID>(componentIDenum::CameraDefaultInput), "DefaultCameraInput", in_ecs_wrapper_ptr),
     default_speed(default_speed)
{
    CameraDefaultInputCompEntity::defaultCameraInputComp_ptr= this;
}

CameraDefaultInputComp::~CameraDefaultInputComp()
{
    CameraDefaultInputCompEntity::defaultCameraInputComp_ptr = nullptr;
}

void CameraDefaultInputComp::Update()
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));
    for (CameraDefaultInputCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.Update(cameraComp_ptr, duration);

    lastSnapTimePoint = next_snap_timePoint;
}

void CameraDefaultInputComp::AsyncInput(InputType input_type, void* struct_data)
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));
    for (CameraDefaultInputCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.AsyncInput(input_type, struct_data, duration);

    lastSnapTimePoint = next_snap_timePoint;
}
