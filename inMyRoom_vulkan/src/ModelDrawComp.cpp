#include "ModelDrawComp.h"
#include "NodeGlobalMatrixComp.h"

#include "ECSwrapper.h"

ModelDrawComp::ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentSparseBaseClass<ModelDrawCompEntity>(static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", in_ecs_wrapper_ptr)
{
    ModelDrawCompEntity::modelDrawComp_ptr = this;
}

ModelDrawComp::~ModelDrawComp()
{
    ModelDrawCompEntity::modelDrawComp_ptr = nullptr;
}

std::vector<DrawRequest> ModelDrawComp::DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr, FrustumCulling* frustemCulling_ptr) const
{
    componentID nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::NodeGlobalMatrix);
    NodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr = static_cast<NodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    std::vector<DrawRequest> draw_requests;
    draw_requests.reserve(componentEntitiesSparse.size());

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].DrawUsingFrustumCull(nodeGlobalMatrixComp_ptr, meshesOfNodes_ptr, frustemCulling_ptr, draw_requests);

    return std::move(draw_requests);
}