#include "ECS/GeneralComponents/CameraDefaultInputComp.h"

#include "ECS/ECSwrapper.h"

CameraDefaultInputComp::CameraDefaultInputComp(ECSwrapper* const in_ecs_wrapper_ptr, float default_speed)
    :ComponentDataClass<CameraDefaultInputCompEntity, static_cast<componentID>(componentIDenum::CameraDefaultInput), "CameraDefaultInput", sparse_set>(in_ecs_wrapper_ptr),
     default_speed(default_speed)
{
}

CameraDefaultInputComp::~CameraDefaultInputComp()
{
}

void CameraDefaultInputComp::Update()
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    componentID camera_componentID = static_cast<componentID>(componentIDenum::Camera);
    CameraComp* const cameraComp_ptr = static_cast<CameraComp*>(ecsWrapper_ptr->GetComponentByID(camera_componentID));

    for (CameraDefaultInputCompEntity& this_componentEntity : componentEntities)
        this_componentEntity.Update(cameraComp_ptr, duration);

    lastSnapTimePoint = next_snap_timePoint;
}

void CameraDefaultInputComp::AsyncInput(InputType input_type, void* struct_data)
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    for (CameraDefaultInputCompEntity& this_componentEntity : componentEntities)
        this_componentEntity.AsyncInput(input_type, struct_data, duration);

    lastSnapTimePoint = next_snap_timePoint;
}
