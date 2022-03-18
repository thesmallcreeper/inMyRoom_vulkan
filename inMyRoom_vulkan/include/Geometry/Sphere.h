#pragma once

#include "glm/vec3.hpp"
#include "Geometry/Paralgram.h"

class Sphere
{
public:
    Sphere(const glm::vec3& origin, float radius);

    glm::vec3 GetOrigin() const {return origin;}
    float GetRadius() const {return radius;}

    bool IntersectParalgram(const Paralgram& paralgram) const;

private:
    glm::vec3 origin;
    float radius;
};