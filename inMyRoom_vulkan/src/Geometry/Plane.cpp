#include "Geometry/Plane.h"

#include <cmath>

Plane::Plane(const glm::vec3& in_normal, float in_d)
{
    glm::vec3 unormalized_normal = in_normal;
    float length = glm::length(unormalized_normal);

    normal = unormalized_normal / length;
    d = in_d / length;
}

Plane Plane::CreatePlaneFromTriangle(const TrianglePosition& triangle)
{
    glm::vec3 normal = triangle.GetTriangleFaceNormal();
    float d = - glm::dot(triangle.GetP(0), normal);

    return Plane(normal, d);
}

PlaneIntersectResult Plane::IntersectPoint(const glm::vec3& point)
{
    float s = glm::dot(point, normal) + d;

    if (s > 0) return PlaneIntersectResult::OUTSIDE;
    else return PlaneIntersectResult::INSIDE;
}

PlaneIntersectResult Plane::IntersectParalgram(const Paralgram& paralgram) const
{
    float e = 0.f;
    e += std::abs( glm::dot(normal , paralgram.GetSideDirectionU()) );
    e += std::abs( glm::dot(normal , paralgram.GetSideDirectionV()) );
    e += std::abs( glm::dot(normal , paralgram.GetSideDirectionW()) );

    float s = glm::dot(paralgram.GetCenter(), normal) + d;

    if (s - e > 0) return PlaneIntersectResult::OUTSIDE;
    if (s + e < 0) return PlaneIntersectResult::INSIDE;
    return PlaneIntersectResult::INTERSECTING;
}

PlaneIntersectResult Plane::IntersectSphere(const Sphere &sphere) const
{
    float s = glm::dot(sphere.GetOrigin(), normal) + d;

    if (s - sphere.GetRadius() > 0) return PlaneIntersectResult::OUTSIDE;
    if (s + sphere.GetRadius() < 0) return PlaneIntersectResult::INSIDE;
    return PlaneIntersectResult::INTERSECTING;
}
