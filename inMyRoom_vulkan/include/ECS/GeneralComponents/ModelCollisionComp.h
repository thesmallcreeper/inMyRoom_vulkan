#pragma once

#include "ECS/GeneralCompEntities/ModelCollisionCompEntity.h"
#include "ECS/ComponentDataClass.h"

#include "ECS/ComponentsIDsEnum.h"

#include "CollisionDetection/CollisionDetection.h"
#include "Graphics/Meshes/MeshesOfNodes.h"

class ModelCollisionComp final
    : public ComponentDataClass<ModelCollisionCompEntity, static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision", sparse_set>
{
public:
    explicit ModelCollisionComp(ECSwrapper* const in_ecs_wrapper_ptr,
                                CollisionDetection* in_collisionDetection_ptr,
                                class MeshesOfNodes* in_meshesOfNodes_ptr);
    ~ModelCollisionComp() override;

    void Update() override;
private:
    CollisionDetection* const collisionDetection_ptr;
    class MeshesOfNodes* const meshesOfNodes_ptr;
};