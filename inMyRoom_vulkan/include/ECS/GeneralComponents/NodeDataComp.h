#pragma once

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/ComponentRawBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"


class NodeDataComp final
    : public ComponentRawBaseClass<NodeDataCompEntity, static_cast<componentID>(componentIDenum::NodeData), "NodeData">
{
public:
    NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~NodeDataComp() override;
};

