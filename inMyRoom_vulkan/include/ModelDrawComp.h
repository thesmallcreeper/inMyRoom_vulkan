#pragma once

#include "ModelDrawCompEntity.h"
#include "ComponentSparseBaseClass.h"

#include "ComponentsIDsEnum.h"

class ModelDrawComp
    :public ComponentSparseBaseClass<ModelDrawCompEntity>
{
public:
    ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ModelDrawComp() override;

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncUpdate(/* to do */) override {};

    std::vector<DrawRequest> DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr, FrustumCulling* frustemCulling_ptr) const;
};

