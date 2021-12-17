#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"
#include "glm/mat4x4.hpp"
#include "Geometry/Triangle.h"
#include "Geometry/OBBtree.h"

struct RayTriangleIntersectInfo
{
    bool doIntersect = false;
    bool itBackfaces = false;
    float distanceFromOrigin = std::numeric_limits<float>::infinity();
    glm::vec2 baryPosition = glm::vec2();
};

struct RayOBBtreeIntersectInfo
{
    bool doIntersect = false;
    bool itBackfaces = false;
    float distanceFromOrigin = std::numeric_limits<float>::infinity();
    size_t triangle_index = -1;
    glm::vec2 baryPosition = glm::vec2();
};

class Ray
{
public:
    Ray() {};
    Ray(const glm::vec3& origin, const glm::vec3& direction);

public:
    glm::vec3 GetOrigin() const {return origin;}
    glm::vec3 GetDirection() const {return direction;}

    void MoveOriginEpsilonTowardsDirection(float factor);

    RayTriangleIntersectInfo IntersectTriangle(const TrianglePosition& triange) const;
    std::pair<bool, std::pair<float, float>> IntersectParalgram(const Paralgram& paralgram) const;        // Returns bool(doIntesept), pair(min, max) distance
    RayOBBtreeIntersectInfo IntersectOBBtree(const OBBtree& obb_tree, const glm::mat4x4& matrix) const;

private:
    void IntersectOBBtreeRecursive(const OBBtree::OBBtreeTraveler& obb_tree_traveler,
                                   const OBBtree& obb_tree,
                                   const glm::mat4x4& matrix,
                                   RayOBBtreeIntersectInfo& best_intersection_so_far) const;
private:
    glm::vec3 origin;
    glm::vec3 direction;
};