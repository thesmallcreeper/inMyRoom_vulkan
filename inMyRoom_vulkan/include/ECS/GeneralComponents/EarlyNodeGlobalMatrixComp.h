#pragma once

#include "ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class EarlyNodeGlobalMatrixComp :
    public ComponentRawBaseClass<EarlyNodeGlobalMatrixCompEntity>
{
public:
    EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~EarlyNodeGlobalMatrixComp() override;

    void Update() override;
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

};

