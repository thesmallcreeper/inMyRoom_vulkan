#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "Cuboid.h"

enum IntersectResult
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
    IntersectResult intersectCuboid(const Cuboid in_cuboid) const;

private:
    glm::vec4 normal;
    float d;
};

