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

    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.Update(cameraComp_ptr, duration);
    }

    lastSnapTimePoint = next_snap_timePoint;
}

void CameraDefaultInputComp::AsyncInput(InputType input_type, void* struct_data)
{
    auto previous_snap_timePoint = lastSnapTimePoint;
    auto next_snap_timePoint = std::chrono::steady_clock::now();

    std::chrono::duration<float> duration = next_snap_timePoint - previous_snap_timePoint;

    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
        {
            this_comp_entity.AsyncInput(input_type, struct_data, duration);
        }
    }

    lastSnapTimePoint = next_snap_timePoint;
}
