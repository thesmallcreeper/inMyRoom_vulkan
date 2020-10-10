#pragma once

#include "ECS/CompEntityBase.h"

#include "ECS/GeneralCompEntities/EarlyNodeGlobalMatrixCompEntity.h"
#include "ECS/GeneralCompEntities/LateNodeGlobalMatrixCompEntity.h"

#ifdef GAME_DLL
class ModelCollisionCompEntity;
class ModelCollisionComp
    :public ComponentBaseWrappedClass<ModelCollisionCompEntity, static_cast<componentID>(componentIDenum::ModelCollision), "ModelCollision"> {};
#else
class ModelCollisionComp;
#endif

class ModelCollisionCompEntity :
    public CompEntityBase<ModelCollisionComp>
{
#ifndef GAME_DLL
public:
    ModelCollisionCompEntity(const Entity this_entity);

    static ModelCollisionCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - ModelDraw
            "MeshIndex",         meshIndex               = int
            "DisableCollision",  disableCollision        = int         (optional)
            "ShouldCallback",    shouldCallback          = int         (optional)
    */
    static ModelCollisionCompEntity CreateComponentEntityByMap(Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

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