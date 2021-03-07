#include "Geometry/Plane.h"

#include <cmath>

Plane::Plane(const glm::vec3 in_normal, const float in_d)
{
    glm::vec3 unormalized_normal = in_normal;
    float length = glm::length(unormalized_normal);

    normal = unormalized_normal / length;
    d = in_d / length;
}

Plane Plane::CreatePlaneFromTriangle(const TrianglePosition& in_triangle)
{
    glm::vec3 normal = in_triangle.GetTriangleNormal();
    float d = - glm::dot(in_triangle.GetP(0), normal);

    return Plane(normal, d);
}

PlaneIntersectResult Plane::IntersectPoint(glm::vec3 in_point)
{
    float s = glm::dot(in_point, this->normal) + this->d;

    if (s > 0) return PlaneIntersectResult::OUTSIDE;
    else return PlaneIntersectResult::INSIDE;
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