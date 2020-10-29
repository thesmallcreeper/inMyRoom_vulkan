#pragma once

#include "ECS/ECStypes.h"
#include "ECS/ECSwrapper.h"

#include "CollisionDetection/SweepAndPrune.h"
#include "CollisionDetection/OBBtreesCollision.h"
#include "CollisionDetection/TrianglesVsTriangles.h"
#include "CollisionDetection/RayDeltaUncollide.h"

class CollisionDetection
{
public:
    CollisionDetection(ECSwrapper* in_ECSwrapper_ptr);

    void Reset();
    void AddCollisionDetectionEntry(const CollisionDetectionEntry in_collisionDetectionEntry);

    void ExecuteCollisionDetection();

private:
    void MakeCallbacks(const std::vector<std::pair<Entity, CollisionCallbackData>>& callback_entity_data_pairs) const;

private:
    std::vector<CollisionDetectionEntry> collisionDetectionEntries;

    std::unique_ptr<SweepAndPrune> broadPhaseCollision_uptr;
    std::unique_ptr<OBBtreesCollision> midPhaseCollision_uptr;
    std::unique_ptr<TrianglesVsTriangles> narrowPhaseCollision_uptr;
    std::unique_ptr<RayDeltaUncollide> rayDeltaUncollide_uptr;

    ECSwrapper* const ECSwrapper_ptr;
};