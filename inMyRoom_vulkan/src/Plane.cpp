#include "Plane.h"

#include <cmath>

Plane Plane::CreatePlane(const glm::vec3 in_normal, const float in_d)
{
    Plane return_plane;

    glm::vec4 unormalized_normal = glm::vec4(in_normal, 0);
    float length = glm::length(unormalized_normal);
    return_plane.normal = unormalized_normal / length;
    return_plane.d = in_d / length;

    return return_plane;
}

IntersectResult Plane::intersectCuboid(const Cuboid in_cuboid) const
{
    glm::vec3 cuboid_halfLengths = in_cuboid.getHalfLengths();

    float e = 0.f;
    e += cuboid_halfLengths.x * std::abs( glm::dot(this->normal , in_cuboid.getSideDirectionU()) );
    e += cuboid_halfLengths.y * std::abs( glm::dot(this->normal , in_cuboid.getSideDirectionV()) );
    e += cuboid_halfLengths.z * std::abs( glm::dot(this->normal , in_cuboid.getSideDirectionW()) );

    float s = glm::dot(in_cuboid.getCenter(), this->normal) + this->d;

    if (s - e > 0) return IntersectResult::OUTSIDE;
    if (s + e < 0) return IntersectResult::INSIDE;
    return IntersectResult::INTERSECTING;
}