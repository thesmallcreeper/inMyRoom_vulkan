#include "CollisionDetection/OBBtreesCollision.h"

OBBtreesCollision::OBBtreesCollision()
{
}

CDentriesPairTrianglesPairs OBBtreesCollision::ExecuteOBBtreesCollision(const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& collisionDetectionEntriesPair) const
{
    CDentriesPairTrianglesPairs return_triangle_pairs;
    return_triangle_pairs.firstEntry = collisionDetectionEntriesPair.first;
    return_triangle_pairs.secondEntry = collisionDetectionEntriesPair.second;

    OBBtree::IntersectOBBtrees(*collisionDetectionEntriesPair.first.OBBtree_ptr,
                               *collisionDetectionEntriesPair.second.OBBtree_ptr,
                               glm::inverse(collisionDetectionEntriesPair.first.currentGlobalMatrix) * collisionDetectionEntriesPair.second.currentGlobalMatrix,
                               return_triangle_pairs.OBBtreesIntersectInfo);

    return return_triangle_pairs;
}
