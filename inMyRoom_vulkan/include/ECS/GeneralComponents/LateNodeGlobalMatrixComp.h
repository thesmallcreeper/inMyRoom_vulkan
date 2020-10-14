#pragma once

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"


class LateNodeGlobalMatrixComp final
    : public ComponentRawBaseClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix">
{
public:
    LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~LateNodeGlobalMatrixComp() override;
};

