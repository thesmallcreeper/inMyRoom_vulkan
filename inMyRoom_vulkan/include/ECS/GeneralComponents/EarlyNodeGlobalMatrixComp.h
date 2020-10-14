#pragma once

#include "ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class EarlyNodeGlobalMatrixComp final
    : public ComponentRawBaseClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix">
{
public:
    EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~EarlyNodeGlobalMatrixComp() override;
};

