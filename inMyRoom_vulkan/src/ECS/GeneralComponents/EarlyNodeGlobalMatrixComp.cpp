#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/NodeDataComp.h"

#include "ECS/ECSwrapper.h"

EarlyNodeGlobalMatrixComp::EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<EarlyNodeGlobalMatrixCompEntity>(static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix", in_ecs_wrapper_ptr)
{
    EarlyNodeGlobalMatrixCompEntity::earlyNodeGlobalMatrixComp_ptr = this;
}

EarlyNodeGlobalMatrixComp::~EarlyNodeGlobalMatrixComp()
{
    EarlyNodeGlobalMatrixCompEntity::earlyNodeGlobalMatrixComp_ptr = nullptr;
}

void EarlyNodeGlobalMatrixComp::Update() //ComponentRawBaseClass
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const positionComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].Update(positionComp_ptr);
}
