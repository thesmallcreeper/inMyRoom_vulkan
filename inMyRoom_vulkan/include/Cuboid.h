#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

class Cuboid
{
public:
    static Cuboid MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Cuboid& rhs);

public:
    glm::vec4 GetCenter() const;

    glm::vec4 GetSideDirectionU() const;
    glm::vec4 GetSideDirectionV() const;
    glm::vec4 GetSideDirectionW() const;

    glm::vec3 GetHalfLengths() const;

protected:
    glm::vec4 center;
    struct
    {
        glm::vec4 u;
        glm::vec4 v;
        glm::vec4 w;
    } sideDirections;
    glm::vec3 halfLengths;
};

inline Cuboid operator* (const glm::mat4x4& in_matrix, const Cuboid& rhs)
{
    return Cuboid::MultiplyBy4x4Matrix(in_matrix, rhs);
}