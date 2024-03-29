#include "ECS/GeneralComponents/ModelCollisionComp.h"

ModelCollisionComp::ModelCollisionComp(ECSwrapper* const in_ecs_wrapper_ptr,
                                       CollisionDetection* in_collisionDetection_ptr,
                                       MeshesOfNodes* in_meshesOfNodes_ptr)
    :ComponentDataClass<ModelCollisionCompEntity, static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision", sparse_set>(in_ecs_wrapper_ptr),
     collisionDetection_ptr(in_collisionDetection_ptr),
     meshesOfNodes_ptr(in_meshesOfNodes_ptr)
{
}

ModelCollisionComp::~ModelCollisionComp()
{
}

void ModelCollisionComp::Update()
{
    collisionDetection_ptr->Reset();

    componentID lateNodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    LateNodeGlobalMatrixComp* const previous_frame_nodeGlobalMatrixComp_ptr = static_cast<LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(lateNodeGlobalMatrix_componentID));

    componentID earlyNodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::EarlyNodeGlobalMatrix);
    EarlyNodeGlobalMatrixComp* const this_frame_nodeGlobalMatrixComp_ptr = static_cast<EarlyNodeGlobalMatrixComp*> (ecsWrapper_ptr->GetComponentByID(earlyNodeGlobalMatrix_componentID));

    size_t containers_count_when_start = GetContainersCount();
    for(; containersUpdated != containers_count_when_start; ++containersUpdated)
    {
        auto& this_container = GetContainerByIndex(containersUpdated);
        for(auto& this_comp_entity: this_container)
            this_comp_entity.AddCollisionDetectionEntryToVector(this_frame_nodeGlobalMatrixComp_ptr,
                                                                previous_frame_nodeGlobalMatrixComp_ptr,
                                                                meshesOfNodes_ptr,
                                                                collisionDetection_ptr);
    }
    collisionDetection_ptr->ExecuteCollisionDetection();
}
