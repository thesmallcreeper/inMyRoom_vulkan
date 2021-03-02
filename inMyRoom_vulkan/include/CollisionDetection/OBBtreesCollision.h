#pragma once

#include "ECS/ECStypes.h"

#include "Geometry/OBBtree.h"

struct CSentriesPairTrianglesPairs
{
    CollisionDetectionEntry firstEntry;
    CollisionDetectionEntry secondEntry;
    OBBtreesIntersectInfo OBBtreesIntersectInfo;
};

class OBBtreesCollision
{
public:
    OBBtreesCollision();

    std::vector<CSentriesPairTrianglesPairs> ExecuteOBBtreesCollision(const std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>>& collisionDetectionEntriesPairs) const;
};