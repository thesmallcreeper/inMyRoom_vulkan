#pragma once

#include "ECS/CompEntityBaseWrappedClass.h"

class ModelCollisionComp;
class ModelCollisionCompEntity;
#ifdef GAME_DLL
class ModelCollisionComp
    :public ComponentBaseWrappedClass<ModelCollisionCompEntity, static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision"> {};
#else
#include "ECS/GeneralComponents/ModelCollisionComp.h"
#endif

#include "ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"
#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

class ModelCollisionCompEntity :
    public CompEntityBaseWrappedClass<ModelCollisionComp>
{
#ifndef GAME_DLL
public:
    ModelCollisionCompEntity(Entity this_entity);

    static ModelCollisionCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - ModelDraw
            "MeshIndex",         meshIndex               = int
            "DisableCollision",  disableCollision        = int         (optional)
            "ShouldCallback",    shouldCallback          = int         (optional)
    */
    static ModelCollisionCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);

    void Init();

    void AddCollisionDetectionEntryToVector(EarlyNodeGlobalMatrixComp* thisFrameNodeGlobalMatrix_ptr,
                                            LateNodeGlobalMatrixComp* previousFrameNodeGlobalMatrix_ptr,
                                            class MeshesOfNodes* meshesOfNodes_ptr,
                                            class CollisionDetection* collisionDetection_ptr) const;

#endif
public:
    uint32_t meshIndex;
    bool disableCollision = false;
    bool shouldCallback = false;
};