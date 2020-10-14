#pragma once

#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "Graphics/Meshes/MeshesOfNodes.h"
#include "Graphics/Meshes/PrimitivesOfMeshes.h"
#include "Geometry/FrustumCulling.h"

struct DrawRequestsBatch
{
    std::vector<DrawRequest> opaqueDrawRequests;
    std::vector<DrawRequest> transparentDrawRequests;
};

class ModelDrawComp final
    : public ComponentSparseBaseClass<ModelDrawCompEntity, static_cast<componentID>(componentIDenum::ModelDraw), "ModelDraw">
{
public:
    ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ModelDrawComp() override;

    DrawRequestsBatch DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr,
                                           PrimitivesOfMeshes* primitivesOfMeshes_ptr,
                                           FrustumCulling* frustemCulling_ptr) const;

};

