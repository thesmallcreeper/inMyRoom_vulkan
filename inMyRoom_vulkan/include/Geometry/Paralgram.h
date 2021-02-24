#pragma once

#include <utility>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"

class Paralgram
{
public:
    static Paralgram MultiplyBy4x4Matrix(const glm::mat4x4& in_matrix, const Paralgram& rhs);

    static bool IntersectParalgramsBoolean(const Paralgram& lhs, const Paralgram& rhs);

    std::pair<float, float> GetMinMaxProjectionToAxis(const glm::vec3& in_axis) const;
    float GetCenterProjectionToAxis(const glm::vec3& in_axis) const;

public:
    float GetSurface() const;

    glm::vec3 GetCenter() const;

    glm::vec3 GetSideDirectionU() const;
    glm::vec3 GetSideDirectionV() const;
    glm::vec3 GetSideDirectionW() const;

private:
    static bool DoMinMaxProjectionsToAxisIntersept(const std::pair<float, float>& lhs, const std::pair<float, float>& rhs);

protected:
    glm::vec3 center;
    struct
    {
        glm::vec3 u;
        glm::vec3 v;
        glm::vec3 w;
    } sideDirections;
};

inline Paralgram operator* (const glm::mat4x4& in_matrix, const Paralgram& rhs)
{
    return Paralgram::MultiplyBy4x4Matrix(in_matrix, rhs);
}

// TODO: remove normalization where it is not needed!