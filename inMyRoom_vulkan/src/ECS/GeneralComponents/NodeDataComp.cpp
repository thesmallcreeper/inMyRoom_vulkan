#include "ECS/GeneralComponents/NodeDataComp.h"

#include "ECS/ECSwrapper.h"

NodeDataComp::NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<NodeDataCompEntity>(static_cast<componentID>(componentIDenum::NodeData), "Position" , in_ecs_wrapper_ptr)
{
    NodeDataCompEntity::positionComp_ptr = this;
}

NodeDataComp::~NodeDataComp()
{
    NodeDataCompEntity::positionComp_ptr = nullptr;
}
