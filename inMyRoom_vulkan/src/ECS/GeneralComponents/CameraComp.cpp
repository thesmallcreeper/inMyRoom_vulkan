#include "ECS/GeneralComponents/CameraComp.h"


CameraComp::CameraComp(ECSwrapper* const in_ecs_wrapper_ptr,
                       float default_fovy, float default_aspect, float default_near, float default_far)
    :
    ComponentDataClass<CameraCompEntity, static_cast<componentID>(componentIDenum::Camera), "Camera", sparse_set>(in_ecs_wrapper_ptr),
    default_fovy(default_fovy),
    default_aspect(default_aspect),
    default_near(default_near),
    default_far(default_far)
{
}

CameraComp::~CameraComp()
{
}

void CameraComp::Update()
{
    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.Update(cullingDebugging);
    }
}

void CameraComp::BindCameraEntity(Entity this_camera_entity)
{
    camera_entity = this_camera_entity;
}

CameraCompEntity* CameraComp::GetBindedCameraEntity()
{
    assert(camera_entity != 0);
    return &GetComponentEntity(camera_entity);
}

void CameraComp::ToggleCullingDebugging()
{
    if (cullingDebugging)
        cullingDebugging = false;
    else
        cullingDebugging = true;
}
