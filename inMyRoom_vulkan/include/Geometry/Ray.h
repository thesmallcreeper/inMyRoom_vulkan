#pragma once

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

class Ray
{
public:
    static Ray CreateRay(const glm::vec3 origin, const glm::vec3 direction);

public:
    glm::vec4 GetOrigin() const;
    glm::vec4 GetDirection() const;

private:
    glm::vec4 origin;
    glm::vec4 direction;
};