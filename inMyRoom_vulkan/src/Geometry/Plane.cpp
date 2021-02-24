#include "Geometry/Plane.h"

#include <cmath>

Plane Plane::CreatePlane(const glm::vec3 in_normal, const float in_d)
{
    Plane return_plane;

    glm::vec3 unormalized_normal = in_normal;
    float length = glm::length(unormalized_normal);
    return_plane.normal = unormalized_normal / length;
    return_plane.d = in_d / length;

    return return_plane;
}

PlaneIntersectResult Plane::IntersectParalgram(const Paralgram in_paralgram) const
{
    float e = 0.f;
    e += std::abs( glm::dot(this->normal , in_paralgram.GetSideDirectionU()) );
    e += std::abs( glm::dot(this->normal , in_paralgram.GetSideDirectionV()) );
    e += std::abs( glm::dot(this->normal , in_paralgram.GetSideDirectionW()) );

    float s = glm::dot(in_paralgram.GetCenter(), this->normal) + this->d;

    if (s - e > 0) return PlaneIntersectResult::OUTSIDE;
    if (s + e < 0) return PlaneIntersectResult::INSIDE;
    return PlaneIntersectResult::INTERSECTING;
}