#include "CollisionDetection/CollisionDetection.h"

#include <iostream>
#include <algorithm>

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
    {
        narrowPhaseCollision_uptr = std::make_unique<TrianglesVsTriangles>();
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

    // Narrow phase collision
    std::vector<CSentriesPairCollisionCenter> narrowPhaseResults = narrowPhaseCollision_uptr->ExecuteTrianglesVsTriangles(midPhaseResults);

    for (const CSentriesPairCollisionCenter& this_narrowPhaseResult : narrowPhaseResults)
    {
        std::string first_name = ECSwrapper_ptr->GetEntitiesHandler()->GetEntityName(std::min<Entity>(this_narrowPhaseResult.firstEntry.entity, this_narrowPhaseResult.secondEntry.entity));
        std::string second_name = ECSwrapper_ptr->GetEntitiesHandler()->GetEntityName(std::max<Entity>(this_narrowPhaseResult.firstEntry.entity, this_narrowPhaseResult.secondEntry.entity));

        std::cout << first_name << "\n";
        std::cout << second_name << "\n";
        std::cout << "x= " << this_narrowPhaseResult.collisionPoint.x << " y= " << this_narrowPhaseResult.collisionPoint.y << " z= " << this_narrowPhaseResult.collisionPoint.z << "\n\n";
    }

    std::cout << "---\n\n";
}
