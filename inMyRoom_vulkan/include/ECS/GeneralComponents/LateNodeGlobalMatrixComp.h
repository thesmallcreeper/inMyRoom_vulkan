#pragma once

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"


class LateNodeGlobalMatrixComp final
    : public ComponentDataClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix", dense_set>
{
public:
    explicit LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~LateNodeGlobalMatrixComp() override;

    void Update() override;
};

