#pragma once

#include "ECS/GeneralCompEntities/PositionCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class PositionComp :
    public ComponentRawBaseClass<PositionCompEntity>
{
public:
    PositionComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~PositionComp() override;

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

};

