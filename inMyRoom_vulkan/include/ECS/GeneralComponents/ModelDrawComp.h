#pragma once

#include "ECS/GeneralCompEntities/ModelDrawCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"



class ModelDrawComp
    :public ComponentSparseBaseClass<ModelDrawCompEntity>
{
public:
    ModelDrawComp(ECSwrapper* const in_ecs_wrapper_ptr);
    ~ModelDrawComp() override;

    void Update() override {};
    void FixedUpdate() override {};
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    std::vector<DrawRequest> DrawUsingFrustumCull(MeshesOfNodes* meshesOfNodes_ptr, FrustumCulling* frustemCulling_ptr) const;
};

