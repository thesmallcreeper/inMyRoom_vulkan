#include "ECS/GeneralComponents/ModelDrawComp.h"

#include "ECS/ECSwrapper.h"

ModelDrawComp::ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr)
    :ComponentDataClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw", sparse_set>(in_ecs_wrapper_ptr)
{
}

ModelDrawComp::~ModelDrawComp()
{
}

DrawRequestsBatch ModelDrawComp::DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr,
                                                      PrimitivesOfMeshes* primitivesOfMeshes_ptr,                 
                                                      FrustumCulling* frustumCulling_ptr) const
{
    componentID nodeGlobalMatrix_componentID = static_cast<componentID>(componentIDenum::LateNodeGlobalMatrix);
    LateNodeGlobalMatrixComp* const nodeGlobalMatrixComp_ptr = static_cast<LateNodeGlobalMatrixComp*>(ecsWrapper_ptr->GetComponentByID(nodeGlobalMatrix_componentID));

    componentID skin_componentID = static_cast<componentID>(componentIDenum::Skin);
    SkinComp* const skinComp_ptr = static_cast<SkinComp*>(ecsWrapper_ptr->GetComponentByID(skin_componentID));

    DrawRequestsBatch this_drawRequestsBatch;
    this_drawRequestsBatch.opaqueDrawRequests.reserve(componentEntities.size());
    this_drawRequestsBatch.transparentDrawRequests.reserve(componentEntities.size() / 6 + 4);

    for (const auto& this_comp_entity: componentEntities)
        this_comp_entity.DrawUsingFrustumCull(nodeGlobalMatrixComp_ptr,
                                              skinComp_ptr,
                                              meshesOfNodes_ptr,
                                              primitivesOfMeshes_ptr,
                                              frustumCulling_ptr,
                                              this_drawRequestsBatch.opaqueDrawRequests,
                                              this_drawRequestsBatch.transparentDrawRequests);

    return std::move(this_drawRequestsBatch);
}

