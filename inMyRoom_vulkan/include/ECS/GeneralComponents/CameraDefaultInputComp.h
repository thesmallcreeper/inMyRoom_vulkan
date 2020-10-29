#pragma once

#include "ECS/GeneralCompEntities/CameraDefaultInputCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"


class CameraDefaultInputComp final
    :public ComponentDataClass<CameraDefaultInputCompEntity, static_cast<componentID>(componentIDenum::CameraDefaultInput), "CameraDefaultInput", sparse_set>
{
public:
    CameraDefaultInputComp(ECSwrapper* const in_ecs_wrapper_ptr, float default_speed);
    ~CameraDefaultInputComp() override;

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override;

public: // data
    const float default_speed;

private:
    std::chrono::steady_clock::time_point lastSnapTimePoint;
};