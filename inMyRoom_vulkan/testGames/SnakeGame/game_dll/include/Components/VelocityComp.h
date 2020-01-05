#pragma once

#include "CompEntities/VelocityCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class VelocityComp :
    public ComponentSparseBaseClass<VelocityCompEntity>
{
public:
    VelocityComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~VelocityComp() override;

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};
};
