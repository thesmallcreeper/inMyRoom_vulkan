#pragma once

#include "glm/vec3.hpp"

#include "Geometry/Paralgram.h"
#include "Geometry/Triangle.h"
#include "Geometry/Sphere.h"

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
    explicit Plane(const glm::vec3& in_normal, float in_d);

    static Plane CreatePlaneFromTriangle(const TrianglePosition& triangle);

    PlaneIntersectResult IntersectPoint(const glm::vec3& point);
    PlaneIntersectResult IntersectParalgram(const Paralgram& in_paralgram) const;
    PlaneIntersectResult IntersectSphere(const Sphere& sphere) const;

private:
    glm::vec3 normal;
    float d;
};

