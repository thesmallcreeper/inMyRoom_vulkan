#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/NodeDataComp.h"

#include "ECS/ECSwrapper.h"

LateNodeGlobalMatrixComp::LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentRawBaseClass<LateNodeGlobalMatrixCompEntity>(static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix", in_ecs_wrapper_ptr)
{
    LateNodeGlobalMatrixCompEntity::lateNodeGlobalMatrixComp_ptr = this;
}

LateNodeGlobalMatrixComp::~LateNodeGlobalMatrixComp()
{
    LateNodeGlobalMatrixCompEntity::lateNodeGlobalMatrixComp_ptr = nullptr;
}

void LateNodeGlobalMatrixComp::Update() //ComponentRawBaseClass
{
    componentID position_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const positionComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    for (size_t index = 0; index < componentEntitiesRaw.size(); index++)
        if (!isItEmptyRawVector[index])
            componentEntitiesRaw[index].Update(positionComp_ptr);
}
