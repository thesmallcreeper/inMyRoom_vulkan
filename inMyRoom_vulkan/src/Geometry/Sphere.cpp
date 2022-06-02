#include "Geometry/Sphere.h"

#include <numbers>

Sphere::Sphere(const glm::vec3 &in_origin, float in_radius)
    :
    origin(in_origin),
    radius(in_radius)
{
}

bool Sphere::IntersectParalgram(const Paralgram& paralgram) const
{
    glm::vec3 sphere_origin = origin - paralgram.GetCenter();

    // U test
    {
        glm::vec3 plane_dir = glm::normalize(glm::cross(paralgram.GetSideDirectionV(), paralgram.GetSideDirectionW()));
        float d = - std::abs(glm::dot(plane_dir, paralgram.GetSideDirectionU()));

        float plane_has_center_dist = glm::dot(plane_dir, sphere_origin);
        float v_n1 = + plane_has_center_dist + d;
        float v_n2 = - plane_has_center_dist + d;

        if (v_n1 - radius > 0 || v_n2 - radius > 0)
            return false;
    }

    // V test
    {
        glm::vec3 plane_dir = glm::normalize(glm::cross(paralgram.GetSideDirectionW(), paralgram.GetSideDirectionU()));
        float d = - std::abs(glm::dot(plane_dir, paralgram.GetSideDirectionV()));

        float plane_has_center_dist = glm::dot(plane_dir, sphere_origin);
        float v_n1 = + plane_has_center_dist + d;
        float v_n2 = - plane_has_center_dist + d;

        if (v_n1 - radius > 0 || v_n2 - radius > 0)
            return false;
    }

    // W test
    {
        glm::vec3 plane_dir = glm::normalize(glm::cross(paralgram.GetSideDirectionU(), paralgram.GetSideDirectionV()));
        float d = - std::abs(glm::dot(plane_dir, paralgram.GetSideDirectionW()));

        float plane_has_center_dist = glm::dot(plane_dir, sphere_origin);
        float v_n1 = + plane_has_center_dist + d;
        float v_n2 = - plane_has_center_dist + d;

        if (v_n1 - radius > 0 || v_n2 - radius > 0)
            return false;
    }

    return true;
}

std::pair<std::vector<uint32_t>, std::vector<glm::vec3>> Sphere::GetSphereMesh(size_t quality)
{
    std::vector<uint32_t> indices;
    std::vector<glm::vec3> points;

    std::vector<uint32_t> previous_level_indices;
    std::vector<uint32_t> this_level_indices;
    for (size_t i = 0; i != quality + 1; ++i) {
        this_level_indices.clear();
        if (i == 0) {
            this_level_indices.emplace_back(points.size()); // points.size() = 0
            points.emplace_back(0.f, 0.f, 1.f);
        } else if (i == quality) {
            size_t last_point_index = points.size();
            this_level_indices.emplace_back(last_point_index);
            points.emplace_back(0.f, 0.f, -1.f);

            for (size_t j = 0; j != previous_level_indices.size(); ++j) {
                uint32_t index_0 = previous_level_indices[(j + 1) % previous_level_indices.size()];
                uint32_t index_1 = previous_level_indices[j];
                uint32_t index_2 = last_point_index;

                indices.emplace_back(index_0);
                indices.emplace_back(index_1);
                indices.emplace_back(index_2);
            }
        } else {
            float theta = float(std::numbers::pi) * (float(i) / float(quality));
            float cos_theta = std::cos(theta);
            float sin_theta = std::sin(theta);

            size_t points_count = 2 * quality;
            for (size_t j = 0; j != points_count; ++j) {
                float phi = 2.f * float(std::numbers::pi) * (float(j) / float(points_count));
                float cos_phi = std::cos(phi);
                float sin_phi = std::sin(phi);

                uint32_t index = points.size();
                this_level_indices.emplace_back(index);
                points.emplace_back(sin_theta * cos_phi,
                                    sin_theta * sin_phi,
                                    cos_theta);
            }

            if (i == 1) {
                for (size_t j = 0; j != this_level_indices.size(); ++j) {
                    uint32_t index_0 = previous_level_indices[0];
                    uint32_t index_1 = this_level_indices[j];
                    uint32_t index_2 = this_level_indices[(j + 1) % this_level_indices.size()];

                    indices.emplace_back(index_0);
                    indices.emplace_back(index_1);
                    indices.emplace_back(index_2);
                }
            } else {
                assert(this_level_indices.size() == previous_level_indices.size());
                for (size_t j = 0; j != this_level_indices.size(); ++j) {
                    uint32_t index_0 = previous_level_indices[(j + 1) % this_level_indices.size()];
                    uint32_t index_1 = previous_level_indices[j];
                    uint32_t index_2 = this_level_indices[j];
                    uint32_t index_3 = this_level_indices[(j + 1) % this_level_indices.size()];

                    // Triangle 0
                    indices.emplace_back(index_0);
                    indices.emplace_back(index_1);
                    indices.emplace_back(index_2);

                    // Triangle 1
                    indices.emplace_back(index_0);
                    indices.emplace_back(index_2);
                    indices.emplace_back(index_3);
                }
            }
        }

        std::swap(previous_level_indices, this_level_indices);
    }

    return std::make_pair(std::move(indices), std::move(points));
}
