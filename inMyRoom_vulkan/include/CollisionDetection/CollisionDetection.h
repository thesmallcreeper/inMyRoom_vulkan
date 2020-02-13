#pragma once

#include "ECS/ECStypes.h"
#include "ECS/ECSwrapper.h"

#include "CollisionDetection/SweepAndPrune.h"
#include "CollisionDetection/OBBtreesCollision.h"
#include "CollisionDetection/TrianglesVsTriangles.h"
#include "CollisionDetection/RayDistance.h"

class CollisionDetection
{
public:
    CollisionDetection(ECSwrapper* in_ECSwrapper_ptr);

    void Reset();
    void AddCollisionDetectionEntry(const CollisionDetectionEntry in_collisionDetectionEntry);

    void ExecuteCollisionDetection();

private:
    std::vector<CollisionDetectionEntry> collisionDetectionEntries;

    std::unique_ptr<SweepAndPrune> broadPhaseCollision_uptr;
    std::unique_ptr<OBBtreesCollision> midPhaseCollision_uptr;
    std::unique_ptr<TrianglesVsTriangles> narrowPhaseCollision_uptr;
    std::unique_ptr<RayDistance> rayDistance_uptr;

    ECSwrapper* const ECSwrapper_ptr;
};