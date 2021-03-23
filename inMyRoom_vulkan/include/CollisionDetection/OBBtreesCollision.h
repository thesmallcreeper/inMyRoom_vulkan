#pragma once

#include "ECS/ECStypes.h"

#include "Geometry/OBBtree.h"

struct CDentriesPairTrianglesPairs
{
    CollisionDetectionEntry firstEntry;
    CollisionDetectionEntry secondEntry;
    OBBtreesIntersectInfo OBBtreesIntersectInfo;
};

class OBBtreesCollision
{
public:
    OBBtreesCollision();

    CDentriesPairTrianglesPairs ExecuteOBBtreesCollision(const std::pair<CollisionDetectionEntry, CollisionDetectionEntry>& collisionDetectionEntriesPair) const;
};