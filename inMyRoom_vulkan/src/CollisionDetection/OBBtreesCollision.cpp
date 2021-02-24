#include "CollisionDetection/OBBtreesCollision.h"

OBBtreesCollision::OBBtreesCollision()
{
}

std::vector<CSentriesPairTrianglesPairs> OBBtreesCollision::ExecuteOBBtreesCollision(const std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>>& collisionDetectionEntriesPairs) const
{
    std::vector<CSentriesPairTrianglesPairs> return_vector;

    for (const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& this_collisionDetectionEntriesPair : collisionDetectionEntriesPairs)
    {
        CSentriesPairTrianglesPairs this_return;
        this_return.firstEntry = this_collisionDetectionEntriesPair.first;
        this_return.secondEntry = this_collisionDetectionEntriesPair.second;

        OBBtree::IntersectOBBtrees(*reinterpret_cast<const OBBtree*>(this_collisionDetectionEntriesPair.first.OBBtree_ptr),
                                   *reinterpret_cast<const OBBtree*>(this_collisionDetectionEntriesPair.second.OBBtree_ptr),
                                   glm::inverse(this_collisionDetectionEntriesPair.first.currentGlobalMatrix) * this_collisionDetectionEntriesPair.second.currentGlobalMatrix,
                                   this_return.OBBtreesIntersectInfo);

        return_vector.emplace_back(this_return);
    }

    return return_vector;
}
