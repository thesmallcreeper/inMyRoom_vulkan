#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

LateNodeGlobalMatrixComp::LateNodeGlobalMatrixComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<LateNodeGlobalMatrixCompEntity, static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix), "LateNodeGlobalMatrix", dense_set>(in_ecs_wrapper_ptr)
{
}

LateNodeGlobalMatrixComp::~LateNodeGlobalMatrixComp()
{
}

void LateNodeGlobalMatrixComp::Update()
{
    EntitiesHandler* entities_handler_ptr = ecsWrapper_ptr->GetEntitiesHandler();

    componentID position_componentID = static_cast<componentID>(componentIDenum::NodeData);
    NodeDataComp* const positionComp_ptr = static_cast<NodeDataComp*>(ecsWrapper_ptr->GetComponentByID(position_componentID));

    componentID earlyNodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    LateNodeGlobalMatrixComp* const earlyNodeGlobalMatrix_ptr = static_cast<LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(earlyNodeGlobalMatrix_componentID));

    for (auto& this_comp_entity: componentEntities)
            this_comp_entity.Update(entities_handler_ptr,
                                    positionComp_ptr,
                                    earlyNodeGlobalMatrix_ptr);
}
