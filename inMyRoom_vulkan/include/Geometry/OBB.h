#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "Geometry/Paralgram.h"
#include "Geometry/Triangle.h"
#include "common/structs/AABB.h"

class OBB: public Paralgram
{
public:
    OBB() = default;
    explicit OBB(const AABB& aabb);

    static OBB EmptyOBB();

    static OBB CreateOBBfromPoints(const std::vector<glm::vec3>& in_points);
    static OBB CreateOBBfromTriangles(const std::vector<Triangle>& in_triangles);

private:
    static OBB CreateAABBfromPoints(const std::vector<glm::vec3>& points,
                                    glm::dvec3 axis_u = glm::dvec3(1.f, 0.f, 0.f),
                                    glm::dvec3 axis_v = glm::dvec3(0.f, 1.f, 0.f),
                                    glm::dvec3 axis_w = glm::dvec3(0.f, 0.f, 1.f));
};
