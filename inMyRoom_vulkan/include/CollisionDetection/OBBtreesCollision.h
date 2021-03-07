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

    CSentriesPairTrianglesPairs ExecuteOBBtreesCollision(const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& collisionDetectionEntriesPair) const;
};