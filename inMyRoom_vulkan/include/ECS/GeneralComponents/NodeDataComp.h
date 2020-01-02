#pragma once

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class NodeDataComp :
    public ComponentRawBaseClass<NodeDataCompEntity>
{
public:
    NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~NodeDataComp() override;

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

};

