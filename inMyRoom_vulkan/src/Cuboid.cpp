#include "Cuboid.h"

Cuboid Cuboid::MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Cuboid& rhs)
{
    Cuboid return_cuboid;

    return_cuboid.center = in_matrix * rhs.center;

    return_cuboid.sideDirections.u = glm::normalize(in_matrix * rhs.sideDirections.u * rhs.halfLengths.x);
    return_cuboid.halfLengths.x = glm::length(in_matrix * rhs.sideDirections.u * rhs.halfLengths.x);
    return_cuboid.sideDirections.v = glm::normalize(in_matrix * rhs.sideDirections.v * rhs.halfLengths.y);
    return_cuboid.halfLengths.y = glm::length(in_matrix * rhs.sideDirections.v * rhs.halfLengths.y);
    return_cuboid.sideDirections.w = glm::normalize(in_matrix * rhs.sideDirections.w * rhs.halfLengths.z);
    return_cuboid.halfLengths.z = glm::length(in_matrix * rhs.sideDirections.w * rhs.halfLengths.z);

    return return_cuboid;
}

glm::vec4 Cuboid::GetCenter() const
{
    return center;
}

glm::vec4 Cuboid::GetSideDirectionU() const
{
    return sideDirections.u;
}
glm::vec4 Cuboid::GetSideDirectionV() const
{
    return sideDirections.v;
}
glm::vec4 Cuboid::GetSideDirectionW() const
{
    return sideDirections.w;
}

glm::vec3 Cuboid::GetHalfLengths() const
{
    return halfLengths;
}
