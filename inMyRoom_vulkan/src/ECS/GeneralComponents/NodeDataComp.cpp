#include "ECS/GeneralComponents/NodeDataComp.h"

#include "ECS/ECSwrapper.h"

NodeDataComp::NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<NodeDataCompEntity>(static_cast<componentID>(componentIDenum::NodeData), "NodeData" , in_ecs_wrapper_ptr)
{
    NodeDataCompEntity::positionComp_ptr = this;
}

NodeDataComp::~NodeDataComp()
{
    NodeDataCompEntity::positionComp_ptr = nullptr;
}

void NodeDataComp::Update()
{
    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].Update();
}
