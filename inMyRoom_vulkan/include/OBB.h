#pragma once

#include <vector>

#include "glm/mat4x4.hpp"
#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "dito.h"

#include "Cuboid.h"

class OBB: public Cuboid
{
public:
    static OBB CreateOBB(const std::vector<glm::vec3>& in_points);
};
