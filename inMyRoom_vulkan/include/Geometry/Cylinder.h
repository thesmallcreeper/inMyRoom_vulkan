#pragma once

#include <vector>
#include <tuple>
#include "glm/vec3.hpp"

class Cylinder
{
public:
    static std::pair<std::vector<uint32_t>, std::vector<glm::vec3>> GetCylinderMesh(size_t quality);
};