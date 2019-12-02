#pragma once

#include "ECS/GeneralCompEntities/CameraCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class CameraComp
    :public ComponentSparseBaseClass<CameraCompEntity>
{
public:
    CameraComp(ECSwrapper* const in_ecs_wrapper_ptr,
               float default_fovy, float default_aspect, float default_near, float default_far);
    ~CameraComp() override;

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void BindCameraEntity(Entity this_camera_entity);
    CameraCompEntity* GetBindedCameraEntity();

    void ToggleCullingDebugging();

public: // data
    const float default_fovy;
    const float default_aspect;
    const float default_near;
    const float default_far;

private:
    Entity camera_entity = 0;
    bool cullingDebugging = false;
};