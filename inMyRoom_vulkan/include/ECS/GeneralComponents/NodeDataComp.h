#pragma once

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"


class NodeDataComp :
    public ComponentRawBaseClass<NodeDataCompEntity, static_cast<componentID>(componentIDenum::NodeData), "NodeData">
{
public:
    NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~NodeDataComp() override;

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};
};

