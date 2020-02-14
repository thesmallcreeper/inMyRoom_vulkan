#include "CollisionDetection/CollisionDetection.h"

CollisionDetection::CollisionDetection(ECSwrapper* in_ECSwrapper_ptr)
    :ECSwrapper_ptr(in_ECSwrapper_ptr)
{
    {
        glm::vec3 U_axis = glm::normalize(glm::vec3(0.8f, -0.2f, 0.f));
        glm::vec3 W_axis = glm::normalize(glm::cross(U_axis, glm::vec3(0.f, -1.f, 0.f)));
        glm::vec3 V_axis = glm::normalize(glm::cross(W_axis, U_axis));

        broadPhaseCollision_uptr = std::make_unique<SweepAndPrune>(U_axis, V_axis, W_axis);
    }
    {
        midPhaseCollision_uptr = std::make_unique<OBBtreesCollision>();
    }
}

void CollisionDetection::Reset()
{
    collisionDetectionEntries.clear();
}

void CollisionDetection::AddCollisionDetectionEntry(const CollisionDetectionEntry in_collisionDetectionEntry)
{
    collisionDetectionEntries.emplace_back(in_collisionDetectionEntry);
}

void CollisionDetection::ExecuteCollisionDetection()
{
    if (collisionDetectionEntries.size() < 2) return;

    // Broad phase collision
    // at least one of the entries should have callback
    std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>> broadPhaseResults = broadPhaseCollision_uptr->ExecuteSweepAndPrune(collisionDetectionEntries);

    // Mid phase collision
    std::vector<CSentriesPairTrianglesPairs> midPhaseResults = midPhaseCollision_uptr->ExecuteOBBtreesCollision(broadPhaseResults);
}
