#pragma once

#include "ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"



class EarlyNodeGlobalMatrixComp final
    : public ComponentDataClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix", dense_set>
{
public:
    EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~EarlyNodeGlobalMatrixComp() override;

    void Update() override;
};

