#pragma once

#include "ECS/GeneralCompEntities/DefaultCameraInputCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"




class DefaultCameraInputComp
    :public ComponentSparseBaseClass<DefaultCameraInputCompEntity>
{
public:
    DefaultCameraInputComp(ECSwrapper* const in_ecs_wrapper_ptr, float default_speed);
    ~DefaultCameraInputComp();

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

public: // data
    const float default_speed;

private:
    std::chrono::steady_clock::time_point lastSnapTimePoint;
};