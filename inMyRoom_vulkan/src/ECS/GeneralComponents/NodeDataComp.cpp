#include "ECS/GeneralComponents/NodeDataComp.h"

#include "ECS/ECSwrapper.h"

NodeDataComp::NodeDataComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<NodeDataCompEntity, static_cast<componentID>(componentIDenum::NodeData), "NodeData">(in_ecs_wrapper_ptr)
{
}

NodeDataComp::~NodeDataComp()
{
}

void NodeDataComp::Update()
{
    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].Update();
}
