#pragma once

#include "ECS/GeneralCompEntities/ModelCollisionCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "CollisionDetection/CollisionDetection.h"
#include "Graphics/Meshes/MeshesOfNodes.h"

class ModelCollisionComp
    :public ComponentSparseBaseClass<ModelCollisionCompEntity, static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision">
{
public:
    ModelCollisionComp(ECSwrapper* const in_ecs_wrapper_ptr,
                       CollisionDetection* in_collisionDetection_ptr,
                       MeshesOfNodes* in_meshesOfNodes_ptr);
    ~ModelCollisionComp() override;

    void Update() override;
    void AsyncInput(InputType input_type, void* struct_data = nullptr) override {};

    void CollisionCallback(Entity this_entity, const CollisionCallbackData& this_collisionCallbackData) override {};
private:
    CollisionDetection* const collisionDetection_ptr;
    MeshesOfNodes* const meshesOfNodes_ptr;
};