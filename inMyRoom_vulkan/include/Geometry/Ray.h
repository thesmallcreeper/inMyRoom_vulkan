#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "Geometry/Triangle.h"
#include "Geometry/OBBtree.h"

struct RayTriangleIntersectInfo
{
    bool doIntersect = false;
    float distanceFromOrigin = std::numeric_limits<float>::infinity();
    Triangle triangle;
};

class Ray
{
public:
    static Ray CreateRay(const glm::vec3 origin, const glm::vec3 direction);

public:
    glm::vec3 GetOrigin() const;
    glm::vec3 GetDirection() const;

    std::pair<bool, float> IntersectTriangle(const Triangle& triange) const;
    std::pair<bool, std::pair<float, float>> IntersectParalgram(const Paralgram& paralgram) const;        // Returns bool(doIntesept), pair(min, max) distance
    RayTriangleIntersectInfo IntersectOBBtree(const OBBtree& obb_tree, const glm::mat4x4& matrix) const;

private:
    void IntersectOBBtreeRecursive(const OBBtree::OBBtreeTraveler& obb_tree_traveler,
                                   const OBBtree& obb_tree,
                                   const glm::mat4x4& matrix,
                                   RayTriangleIntersectInfo& best_intersection_so_far) const;
private:
    glm::vec3 origin;
    glm::vec3 direction;
};