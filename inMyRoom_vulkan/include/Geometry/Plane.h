#pragma once

#include "glm/vec3.hpp"

#include "Geometry/Paralgram.h"

enum PlaneIntersectResult
{
    OUTSIDE,
    INSIDE,
    INTERSECTING
};

class Plane
{
public:
    static Plane CreatePlane(const glm::vec3 in_normal, const float in_d);

public:
    PlaneIntersectResult IntersectParalgram(const Paralgram in_paralgram) const;

private:
    glm::vec3 normal;
    float d;
};

