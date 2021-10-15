#pragma once

#include "ECS/GeneralCompEntities/NodeDataCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"


class NodeDataComp final
    : public ComponentDataClass<NodeDataCompEntity, static_cast<componentID>(componentIDenum::NodeData), "NodeData", dense_set>
{
public:
    explicit NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~NodeDataComp() override;
};

