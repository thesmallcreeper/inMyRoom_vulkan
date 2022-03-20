#pragma once

#include <vector>
#include <tuple>

#include "glm/vec3.hpp"
#include "Geometry/Paralgram.h"

class Sphere
{
public:
    Sphere(const glm::vec3& origin, float radius);

    glm::vec3 GetOrigin() const {return origin;}
    float GetRadius() const {return radius;}

    bool IntersectParalgram(const Paralgram& paralgram) const;

    static std::pair<std::vector<uint32_t>, std::vector<glm::vec3>> GetSphereMesh(size_t quality);

private:
    glm::vec3 origin;
    float radius;
};