#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

EarlyNodeGlobalMatrixComp::EarlyNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<EarlyNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix), "EarlyNodeGlobalMatrix", dense_set>(in_ecs_wrapper_ptr)
{
}

EarlyNodeGlobalMatrixComp::~EarlyNodeGlobalMatrixComp()
{
}

void EarlyNodeGlobalMatrixComp::Update()
{
    EntitiesHandler* entities_handler_ptr = ecsWrapper_ptr->GetEntitiesHandler();

    componentID position_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const positionComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    componentID earlyNodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix);
    EarlyNodeGlobalMatrixComp* const earlyNodeGlobalMatrix_ptr = static_cast<EarlyNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(earlyNodeGlobalMatrix_componentID));

    for(auto& this_comp_entity: componentEntities)
        this_comp_entity.Update(entities_handler_ptr,
                                positionComp_ptr,
                                earlyNodeGlobalMatrix_ptr);
}
