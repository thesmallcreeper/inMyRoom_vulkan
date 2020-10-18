#include "ECS/GeneralCompEntities/ModelCollisionCompEntity.h"

#include "ECS/ECSwrapper.h"

#ifndef GAME_DLL

#include "Graphics/Meshes/MeshesOfNodes.h"
#include "CollisionDetection/CollisionDetection.h"

ModelCollisionCompEntity::ModelCollisionCompEntity(const Entity this_entity)
    :CompEntityBaseWrappedClass<ModelCollisionComp>(this_entity)
{
}

ModelCollisionCompEntity ModelCollisionCompEntity::GetEmpty()
{
    ModelCollisionCompEntity this_modelCollisionCompEntity(0);

    return this_modelCollisionCompEntity;
}

ModelCollisionCompEntity ModelCollisionCompEntity::CreateComponentEntityByMap(const Entity in_entity, const CompEntityInitMap& in_map)
{
    ModelCollisionCompEntity this_modelCollisionCompEntity(in_entity);

    // "MeshIndex", meshIndex = int
    {
        auto search = in_map.intMap.find("MeshIndex");
        assert(search != in_map.intMap.end());

        int this_int = search->second;
        this_modelCollisionCompEntity.meshIndex = static_cast<uint32_t>(this_int);
    }
    // "DisableCollision", disableCollision = int  (optional)
    {
        auto search = in_map.intMap.find("DisableCollision");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_modelCollisionCompEntity.disableCollision = static_cast<bool>(this_int);
        }
    }
    // "ShouldCallback", shouldCallback   = int  (optional)
    {
        auto search = in_map.intMap.find("ShouldCallback");
        if (search != in_map.intMap.end())
        {
            int this_int = search->second;
            this_modelCollisionCompEntity.shouldCallback = static_cast<bool>(this_int);
        }
    }

    return this_modelCollisionCompEntity;
}

void ModelCollisionCompEntity::Init()
{}

void ModelCollisionCompEntity::AddCollisionDetectionEntryToVector(EarlyNodeGlobalMatrixComp* thisFrameNodeGlobalMatrix_ptr,
                                                                  LateNodeGlobalMatrixComp* previousFrameNodeGlobalMatrix_ptr,
                                                                  MeshesOfNodes* meshesOfNodes_ptr,
                                                                  CollisionDetection* collisionDetection_ptr) const
{
    if (!disableCollision)
    {
        EarlyNodeGlobalMatrixCompEntity& this_thisFrameNodeGlobalMatrix_ptr = thisFrameNodeGlobalMatrix_ptr->GetComponentEntity(thisEntity);
        const glm::mat4x4 this_frame_global_matrix = this_thisFrameNodeGlobalMatrix_ptr.globalMatrix;

        LateNodeGlobalMatrixCompEntity& this_previousFrameNodeGlobalMatrix_ptr = previousFrameNodeGlobalMatrix_ptr->GetComponentEntity(thisEntity);
        const glm::mat4x4 previous_frame_global_matrix = this_previousFrameNodeGlobalMatrix_ptr.globalMatrix;

        CollisionDetectionEntry this_collisionDetectionEntry;
        this_collisionDetectionEntry.currentGlobalMatrix = this_frame_global_matrix;
        this_collisionDetectionEntry.previousGlobalMatrix = previous_frame_global_matrix;
        this_collisionDetectionEntry.shouldCallback = shouldCallback;
        this_collisionDetectionEntry.OBBtree_ptr = reinterpret_cast<const void*>(&(meshesOfNodes_ptr->GetMeshInfoPtr(meshIndex)->boundBoxTree));
        this_collisionDetectionEntry.entity = thisEntity;

        collisionDetection_ptr->AddCollisionDetectionEntry(this_collisionDetectionEntry);
    }
}

#endif
