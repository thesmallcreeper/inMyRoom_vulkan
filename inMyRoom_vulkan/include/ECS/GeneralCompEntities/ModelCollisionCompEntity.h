#pragma once

#include "ECS/ECStypes.h"

#include "Graphics/Meshes/MeshesOfNodes.h"
#include "CollisionDetection/CollisionDetection.h"

#ifndef GAME_DLL
class ModelCollisionComp;
#endif

class ModelCollisionCompEntity
{
#ifndef GAME_DLL
public:
    ModelCollisionCompEntity(const Entity this_entity);
    ~ModelCollisionCompEntity();

    static ModelCollisionCompEntity GetEmpty();

    /*  CreateComponentEntityByMap - ModelDraw
            "MeshIndex",         meshIndex               = int
            "DisableCollision",  disableCollision        = int         (optional)
            "ShouldCallback",    shouldCallback          = int         (optional)
    */
    static ModelCollisionCompEntity CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map);
    static std::vector<std::pair<std::string, MapType>> GetComponentInitMapFields();

    void Init();

    void AddCollisionDetectionEntryToVector(class EarlyNodeGlobalMatrixComp* thisFrameNodeGlobalMatrix_ptr,
                                            class LateNodeGlobalMatrixComp* previousFrameNodeGlobalMatrix_ptr,
                                            MeshesOfNodes* meshesOfNodes_ptr,
                                            CollisionDetection* collisionDetection_ptr) const;

private: // static variable
    friend class ModelCollisionComp;
    static ModelCollisionComp* modelCollisionComp_ptr;
#endif
public:
    uint32_t meshIndex;
    bool disableCollision = false;
    bool shouldCallback = false;

    Entity thisEntity;
};