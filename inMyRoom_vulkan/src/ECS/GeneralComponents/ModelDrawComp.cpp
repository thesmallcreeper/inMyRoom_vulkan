#include "ECS/GeneralComponents/ModelDrawComp.h"
#include "ECS/GeneralComponents/NodeGlobalMatrixComp.h"

#include "ECS/ECSwrapper.h"

ModelDrawComp::ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentSparseBaseClass<ModelDrawCompEntity>(static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", in_ecs_wrapper_ptr)
{
    ModelDrawCompEntity::modelDrawComp_ptr = this;
}

ModelDrawComp::~ModelDrawComp()
{
    ModelDrawCompEntity::modelDrawComp_ptr = nullptr;
}

std::vector<std::pair<std::string, MapType>> ModelDrawComp::GetComponentInitMapFields()
{
    return ModelDrawCompEntity::GetComponentInitMapFields();
}

DrawRequestsBatch ModelDrawComp::DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr,
                                                      PrimitivesOfMeshes* primitivesOfMeshes_ptr,
                                                      FrustumCulling* frustemCulling_ptr) const
{
    componentID nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::NodeGlobalMatrix);
    NodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr = static_cast<NodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    DrawRequestsBatch this_drawRequestsBatch;
    this_drawRequestsBatch.opaqueDrawRequests.reserve(componentEntitiesSparse.size());
    this_drawRequestsBatch.transparentDrawRequests.reserve(componentEntitiesSparse.size() / 6 + 4);

    for (size_t index = 0; index < componentEntitiesSparse.size(); index++)
        componentEntitiesSparse[index].DrawUsingFrustumCull(nodeGlobalMatrixComp_ptr, meshesOfNodes_ptr, primitivesOfMeshes_ptr, frustemCulling_ptr,
                                                            this_drawRequestsBatch.opaqueDrawRequests, this_drawRequestsBatch.transparentDrawRequests);

    return std::move(this_drawRequestsBatch);
}