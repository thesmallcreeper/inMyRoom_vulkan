#pragma once

#include "ECS/ECStypes.h"

#include "Geometry/OBBtree.h"

typedef std::vector<Triangle> Triangles;

struct CSentriesPairTrianglesPairs
{
    CollisionDetectionEntry firstEntry;
    CollisionDetectionEntry secondEntry;
    std::vector<std::pair<Triangles, Triangles>> trianglesPairs;
};

class OBBtreesCollision
{
public:
    OBBtreesCollision();

    std::vector<CSentriesPairTrianglesPairs> ExecuteOBBtreesCollision(const std::vector<std::pair<CollisionDetectionEntry, CollisionDetectionEntry>>& collisionDetectionEntriesPairs) const;

private:
    void OBBtreesCollisionHotloop(const std::pair<const OBBtree*, const OBBtree*> OBBtrees_ptrs_pair,
                                  const std::pair<const glm::mat4x4, const glm::mat4x4>& matrices_pair,
                                  std::vector<std::pair<Triangles, Triangles>>& trianglesPairs) const;
};