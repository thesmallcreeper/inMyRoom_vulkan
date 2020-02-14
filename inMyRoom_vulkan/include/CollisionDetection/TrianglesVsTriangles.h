#pragma once

#include "CollisionDetection/OBBtreesCollision.h"

struct CSentriesPairCollisionCenter
{
    CollisionDetectionEntry firstEntry;
    CollisionDetectionEntry secondEntry;
    glm::vec3 collisionPoint;
};

class TrianglesVsTriangles
{
public:
    TrianglesVsTriangles();

    std::vector<CSentriesPairCollisionCenter> ExecuteTrianglesVsTriangles(const std::vector<CSentriesPairTrianglesPairs>& CSentriesPairsTrianglesPairs) const;
};