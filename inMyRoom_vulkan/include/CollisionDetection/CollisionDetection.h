#pragma once

#include "ECS/ECStypes.h"
#include "ECS/ECSwrapper.h"

#include "CollisionDetection/SweepAndPrune.h"
#include "CollisionDetection/OBBtreesCollision.h"
#include "CollisionDetection/CreateUncollideRays.h"
#include "CollisionDetection/ShootUncollideRays.h"

class CollisionDetection
{
public:
    CollisionDetection(ECSwrapper* in_ECSwrapper_ptr);

    void Reset();
    void AddCollisionDetectionEntry(const CollisionDetectionEntry in_collisionDetectionEntry);

    void ExecuteCollisionDetection();

private:
    void MakeCallbacks(std::unordered_map<Entity, std::vector<CollisionCallbackData>>&& callbacks_to_be_made) const;

    float PointMovementBetweenFrames(glm::vec3 point, const glm::mat4& m_first, const glm::mat4& m_second);

private:
    std::vector<CollisionDetectionEntry> collisionDetectionEntries;

    std::unique_ptr<SweepAndPrune> broadPhaseCollision_uptr;
    std::unique_ptr<OBBtreesCollision> midPhaseCollision_uptr;
    std::unique_ptr<CreateUncollideRays> createUncollideRays_uptr;
    std::unique_ptr<ShootUncollideRays> shootDeltaUncollide_uptr;

    ECSwrapper* const ECSwrapper_ptr;
};