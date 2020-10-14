#pragma once

#include "ECS/GeneralCompEntities/ModelCollisionCompEntity.h"
#include "ECS/ComponentSparseBaseClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "CollisionDetection/CollisionDetection.h"
#include "Graphics/Meshes/MeshesOfNodes.h"

class ModelCollisionComp final
    : public ComponentSparseBaseClass<ModelCollisionCompEntity, static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision">
{
public:
    ModelCollisionComp(ECSwrapper* const in_ecs_wrapper_ptr,
                       CollisionDetection* in_collisionDetection_ptr,
                       class MeshesOfNodes* in_meshesOfNodes_ptr);
    ~ModelCollisionComp() override;

    void Update() override;
private:
    CollisionDetection* const collisionDetection_ptr;
    class MeshesOfNodes* const meshesOfNodes_ptr;
};