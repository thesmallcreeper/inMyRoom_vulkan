#pragma once

#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

struct DrawRequestsBatch
{
    std::vector<DrawRequest> opaqueDrawRequests;
    std::vector<DrawRequest> transparentDrawRequests;
};

class ModelDrawComp
    :public ComponentSparseBaseClass<ModelDrawCompEntity>
{
public:
    ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ModelDrawComp() override;

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};

    DrawRequestsBatch DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr,
                                           PrimitivesOfMeshes* primitivesOfMeshes_ptr,
                                           FrustumCulling* frustemCulling_ptr) const;

};

