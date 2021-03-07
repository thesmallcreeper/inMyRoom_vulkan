#pragma once

#include "glm/vec3.hpp"

#include "Geometry/Paralgram.h"
#include "Geometry/Triangle.h"

enum class PlaneIntersectResult
{
    OUTSIDE,
    INSIDE,
    INTERSECTING
};

class Plane
{
public:
    Plane() {}
    explicit Plane(const glm::vec3 in_normal, const float in_d);

    static Plane CreatePlaneFromTriangle(const TrianglePosition& in_triangle);

    PlaneIntersectResult IntersectPoint(glm::vec3 in_point);
    PlaneIntersectResult IntersectParalgram(const Paralgram in_paralgram) const;

private:
    glm::vec3 normal;
    float d;
};

