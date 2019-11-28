#pragma once

#include "ECS/ComponentRawBaseClass.h"
#include "ECS/GeneralCompEntities/PositionCompEntity.h"

#include "ECS/ComponentsIDsEnum.h"

class PositionComp :
    public ComponentRawBaseClass<PositionCompEntity>
{
public:
    PositionComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~PositionComp() override;

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncUpdate(/* to do */) override {};
};

