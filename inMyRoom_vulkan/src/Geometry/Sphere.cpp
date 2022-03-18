#include "Geometry/Sphere.h"

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