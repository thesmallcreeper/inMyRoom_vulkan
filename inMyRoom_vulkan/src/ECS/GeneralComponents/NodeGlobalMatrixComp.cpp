#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/PositionComp.h"

#include "ECS/ECSwrapper.h"

NodeGlobalMatrixComp::NodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<NodeGlobalMatrixCompEntity>(static_cast<componentID>(componentIDenum::NodeGlobalMatrix), "NodeGlobalMatrix", in_ecs_wrapper_ptr)
{
    NodeGlobalMatrixCompEntity::nodeGlobalMatrixComp_ptr = this;
}

NodeGlobalMatrixComp::~NodeGlobalMatrixComp()
{
    NodeGlobalMatrixCompEntity::nodeGlobalMatrixComp_ptr = nullptr;
}

std::vector<std::pair<std::string, MapType>> NodeGlobalMatrixComp::GetComponentInitMapFields()
{
    return NodeGlobalMatrixCompEntity::GetComponentInitMapFields();
}

void NodeGlobalMatrixComp::Update() //ComponentRawBaseClass
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::Position);
    PositionComp* const positionComp_ptr = static_cast<PositionComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].Update(positionComp_ptr);
}
