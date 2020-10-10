#pragma once

#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"


class LateNodeGlobalMatrixComp :
    public ComponentRawBaseClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix">
{
public:
    LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~LateNodeGlobalMatrixComp() override;

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};
};

