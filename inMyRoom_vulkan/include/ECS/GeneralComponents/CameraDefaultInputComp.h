#pragma once

#include "ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"




class CameraDefaultInputComp
    :public ComponentSparseBaseClass<CameraDefaultInputCompEntity>
{
public:
    CameraDefaultInputComp(ECSwrapper* const in_ecs_wrapper_ptr, float default_speed);
    ~CameraDefaultInputComp();

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};

public: // data
    const float default_speed;

private:
    std::chrono::steady_clock::time_point lastSnapTimePoint;
};