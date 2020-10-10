#include "ECS/GeneralComponents/CameraComp.h"


CameraComp::CameraComp(ECSwrapper* const in_ecs_wrapper_ptr,
                       float default_fovy, float default_aspect, float default_near, float default_far)
    :
    ComponentSparseBaseClass<CameraCompEntity, static_cast<componentID>(componentIDenum::Camera), "Camera">(in_ecs_wrapper_ptr),
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
    for (CameraCompEntity& this_componentEntity : componentEntitiesSparse)
        this_componentEntity.Update(cullingDebugging);
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
