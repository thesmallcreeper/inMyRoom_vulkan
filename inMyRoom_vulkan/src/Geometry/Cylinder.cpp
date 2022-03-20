#include "Geometry/Cylinder.h"

#include <cmath>

std::pair<std::vector<uint32_t>, std::vector<glm::vec3>> Cylinder::GetCylinderMesh(size_t quality)
{
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> points;

    size_t points_count = quality * 2;
    std::vector<uint32_t> upper_indices;
    std::vector<uint32_t> lower_indices;
    for (size_t i = 0; i != points_count; ++i) {
        size_t upper_index = points.size();
        size_t lower_index = points.size() + 1;

        float phi = 2.f * float(M_PI) * (float(i) / float(points_count));
        float cos_phi = std::cos(phi);
        float sin_phi = std::sin(phi);

        points.emplace_back(+1.f,
                            cos_phi,
                            sin_phi);
        points.emplace_back(-1.f,
                            cos_phi,
                            sin_phi);

        upper_indices.emplace_back(upper_index);
        lower_indices.emplace_back(lower_index);
    }

    assert(upper_indices.size() == lower_indices.size());
    for (size_t i = 0; i != upper_indices.size(); ++i) {
        uint32_t index_0 = upper_indices[(i + 1) % upper_indices.size()];
        uint32_t index_1 = upper_indices[i];
        uint32_t index_2 = lower_indices[i];
        uint32_t index_3 = lower_indices[(i + 1) % upper_indices.size()];

        // Triangle 0
        indices.emplace_back(index_0);
        indices.emplace_back(index_1);
        indices.emplace_back(index_2);

        // Triangle 1
        indices.emplace_back(index_0);
        indices.emplace_back(index_2);
        indices.emplace_back(index_3);
    }

    return std::make_pair(std::move(indices), std::move(points));
}
