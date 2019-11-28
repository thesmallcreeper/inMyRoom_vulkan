#pragma once

#include "ECS/GeneralCompEntities/NodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

class NodeGlobalMatrixComp :
    public ComponentRawBaseClass<NodeGlobalMatrixCompEntity>
{
public:
    NodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~NodeGlobalMatrixComp() override;

    void Update() override;
    void FixedUpdate() override {};
    void AsyncUpdate(/* to do */) override {};
};

