#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "Geometry/Paralgram.h"
#include "Geometry/Triangle.h"

class OBB: public Paralgram
{
public:
    static OBB EmptyOBB();

    static OBB CreateOBBfromPoints(const std::vector<glm::vec3>& in_points);
    static OBB CreateOBBfromTriangles(const std::vector<Triangle>& in_triangles);

private:
    // in cases when DiTO_14 return garbages
    static OBB CreateAABBfromPoints(const std::vector<glm::vec3>& in_points);
};
