#include "ECS/GeneralComponents/ModelCollisionComp.h"

#include "ECS/GeneralComponents/EarlyNodeGlobalMatrixComp.h"
#include "ECS/GeneralComponents/LateNodeGlobalMatrixComp.h"

ModelCollisionComp::ModelCollisionComp(ECSwrapper* const in_ecs_wrapper_ptr,
                                       CollisionDetection* in_collisionDetection_ptr,
                                       MeshesOfNodes* in_meshesOfNodes_ptr)
    :ComponentSparseBaseClass<ModelCollisionCompEntity>(static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision", in_ecs_wrapper_ptr),
     collisionDetection_ptr(in_collisionDetection_ptr),
     meshesOfNodes_ptr(in_meshesOfNodes_ptr)
{
    ModelCollisionCompEntity::modelCollisionComp_ptr = this;
}

ModelCollisionComp::~ModelCollisionComp()
{
    ModelCollisionCompEntity::modelCollisionComp_ptr = nullptr;
}

void ModelCollisionComp::Update()
{
    collisionDetection_ptr->Reset();

    componentID lateNodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    LateNodeGlobalMatrixComp* const previous_frame_nodeGlobalMatrixComp_ptr = static_cast<LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(lateNodeGlobalMatrix_componentID));

    componentID earlyNodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix);
    EarlyNodeGlobalMatrixComp* const this_frame_nodeGlobalMatrixComp_ptr = static_cast<EarlyNodeGlobalMatrixComp*> (ecsWrapper_ptr->GetComponentByID(earlyNodeGlobalMatrix_componentID));

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
    {
        componentEntitiesSparse[index].AddCollisionDetectionEntryToVector(this_frame_nodeGlobalMatrixComp_ptr,
                                                                          previous_frame_nodeGlobalMatrixComp_ptr,
                                                                          meshesOfNodes_ptr,
                                                                          collisionDetection_ptr);
    }

    collisionDetection_ptr->ExecuteCollisionDetection();
}
