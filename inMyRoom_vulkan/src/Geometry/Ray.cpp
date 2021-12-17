#include "Geometry/Ray.h"
#include "glm/gtx/intersect.hpp"

#include <algorithm>

Ray::Ray(const glm::vec3& in_origin, const glm::vec3& in_direction)
    :
    origin(in_origin),
    direction(in_direction)
{
}

glm::vec3 Ray::GetOrigin() const
{
    return origin;
}

glm::vec3 Ray::GetDirection() const
{
    return direction;
}

void Ray::MoveOriginEpsilonTowardsDirection(float factor)
{
    float biggest_component_abs_value = std::max({std::abs(origin.x), std::abs(origin.y), std::abs(origin.z)});
    float scalled_epsilon = biggest_component_abs_value * std::numeric_limits<float>::epsilon();

    glm::vec3 origin_tick = direction * (factor * scalled_epsilon);

    origin = origin + origin_tick;
}

RayTriangleIntersectInfo Ray::IntersectTriangle(const TrianglePosition& triange) const
{
    RayTriangleIntersectInfo return_info;

    glm::vec3 _p0 = triange.GetP(0);
    glm::vec3 _p1 = triange.GetP(1);
    glm::vec3 _p2 = triange.GetP(2);

    return_info.doIntersect = glm::intersectRayTriangle(origin, direction, _p0, _p1, _p2, return_info.baryPosition, return_info.distanceFromOrigin, return_info.itBackfaces);

    return return_info;
}

std::pair<bool, std::pair<float, float>> Ray::IntersectParalgram(const Paralgram& paralgram) const
{
    // Code based on Eric Haines, "Fast Ray-Convex polyhedron intersection"

    float min_distance = -std::numeric_limits<float>::infinity();
    float max_distance = +std::numeric_limits<float>::infinity();

    glm::vec3 ray_origin = origin - paralgram.GetCenter();

    // U test
    {
        glm::vec3 plane_dir = glm::normalize(glm::cross(paralgram.GetSideDirectionV(), paralgram.GetSideDirectionW()));
        float d = - std::abs(glm::dot(plane_dir, paralgram.GetSideDirectionU()));

        float plane_has_center_dist = glm::dot(plane_dir, ray_origin);
        float v_n1 = + plane_has_center_dist + d;
        float v_n2 = - plane_has_center_dist + d;

        float vd = glm::dot(plane_dir, direction);

        if (std::abs(vd) >= 0.e-8f) [[likely]] {

            float vd_inv = 1.f / vd;
            float t1 = - v_n1 * vd_inv;
            float t2 = + v_n2 * vd_inv;

            if (t1 > t2) std::swap(t1, t2);

            min_distance = std::max(t1, min_distance);
            max_distance = std::min(t2, max_distance);

            if (min_distance > max_distance || max_distance < 0.f)
                return {false, {0.f, 0.f}};

        } else if ( v_n1 > 0 || v_n2 > 0)
            return {false, {0.f, 0.f}};
    }

    // V test
    {
        glm::vec3 plane_dir = glm::normalize(glm::cross(paralgram.GetSideDirectionW(), paralgram.GetSideDirectionU()));
        float d = - std::abs(glm::dot(plane_dir, paralgram.GetSideDirectionV()));

        float plane_has_center_dist = glm::dot(plane_dir, ray_origin);
        float v_n1 = + plane_has_center_dist + d;
        float v_n2 = - plane_has_center_dist + d;

        float vd = glm::dot(plane_dir, direction);

        if (std::abs(vd) >= 0.e-8f) [[likely]] {

            float vd_inv = 1.f / vd;
            float t1 = - v_n1 * vd_inv;
            float t2 = + v_n2 * vd_inv;

            if (t1 > t2) std::swap(t1, t2);

            min_distance = std::max(t1, min_distance);
            max_distance = std::min(t2, max_distance);

            if (min_distance > max_distance || max_distance < 0.f)
                return {false, {0.f, 0.f}};

        } else if ( v_n1 > 0 || v_n2 > 0)
            return {false, {0.f, 0.f}};
    }

    // W test
    {
        glm::vec3 plane_dir = glm::normalize(glm::cross(paralgram.GetSideDirectionU(), paralgram.GetSideDirectionV()));
        float d = - std::abs(glm::dot(plane_dir, paralgram.GetSideDirectionW()));

        float plane_has_center_dist = glm::dot(plane_dir, ray_origin);
        float v_n1 = + plane_has_center_dist + d;
        float v_n2 = - plane_has_center_dist + d;

        float vd = glm::dot(plane_dir, direction);

        if (std::abs(vd) >= 0.e-8f) [[likely]] {

            float vd_inv = 1.f / vd;
            float t1 = - v_n1 * vd_inv;
            float t2 = + v_n2 * vd_inv;

            if (t1 > t2) std::swap(t1, t2);

            min_distance = std::max(t1, min_distance);
            max_distance = std::min(t2, max_distance);

            if (min_distance > max_distance || max_distance < 0.f)
                return {false, {0.f, 0.f}};

        } else if ( v_n1 > 0 || v_n2 > 0)
            return {false, {0.f, 0.f}};
    }

    return {true, {min_distance, max_distance}};
}

RayOBBtreeIntersectInfo Ray::IntersectOBBtree(const OBBtree& obb_tree, const glm::mat4x4& matrix) const
{

    if (origin == glm::vec3(0.f, 0.f, 0.f)) {
        RayOBBtreeIntersectInfo return_intersect_info = {};

        OBBtree::OBBtreeTraveler root_traveler = obb_tree.GetRootTraveler();

        Paralgram root_paralgram = matrix * root_traveler.GetOBB();
        std::pair<bool, std::pair<float, float>> root_paralgram_intersect = IntersectParalgram(root_paralgram);

        if (root_paralgram_intersect.first && root_paralgram_intersect.second.second >= 0.f) {
            IntersectOBBtreeRecursive(root_traveler, obb_tree, matrix, return_intersect_info);
        }

        return return_intersect_info;
    } else {
        Ray centered_ray = {glm::vec3(0.f, 0.f, 0.f) , direction};
        glm::mat4x4 centered_matrix = matrix;
        centered_matrix[3] -= glm::vec4(origin, 0.f);

        return centered_ray.IntersectOBBtree(obb_tree, centered_matrix);
    }
   

}

void Ray::IntersectOBBtreeRecursive(const OBBtree::OBBtreeTraveler& obb_tree_traveler,
                           const OBBtree& obb_tree,
                           const glm::mat4x4& matrix,
                           RayOBBtreeIntersectInfo& best_intersection_so_far_info) const
{
    if (not obb_tree_traveler.IsLeaf())
    {
        OBBtree::OBBtreeTraveler left_child_traveler = obb_tree_traveler.GetLeftChildTraveler();
        OBBtree::OBBtreeTraveler right_child_traveler = obb_tree_traveler.GetRightChildTraveler();

        Paralgram left_paralgram = matrix * left_child_traveler.GetOBB();
        Paralgram right_paralgram = matrix * right_child_traveler.GetOBB();

        std::pair<bool, std::pair<float, float>> left_paralgram_booleanMinMax = IntersectParalgram(left_paralgram);
        std::pair<bool, std::pair<float, float>> right_paralgram_booleanMinMax = IntersectParalgram(right_paralgram);

        if (left_paralgram_booleanMinMax.first && right_paralgram_booleanMinMax.first)
        {
            if (left_paralgram_booleanMinMax.second.first < right_paralgram_booleanMinMax.second.first)
            {
                if (left_paralgram_booleanMinMax.second.first < best_intersection_so_far_info.distanceFromOrigin && left_paralgram_booleanMinMax.second.second >= 0.f)
                {
                    IntersectOBBtreeRecursive(left_child_traveler, obb_tree, matrix, best_intersection_so_far_info);
                }
                if (right_paralgram_booleanMinMax.second.first < best_intersection_so_far_info.distanceFromOrigin && right_paralgram_booleanMinMax.second.second >= 0.f)
                {
                    IntersectOBBtreeRecursive(right_child_traveler, obb_tree, matrix, best_intersection_so_far_info);
                }
            }
            else
            {
                if (right_paralgram_booleanMinMax.second.first < best_intersection_so_far_info.distanceFromOrigin && right_paralgram_booleanMinMax.second.second >= 0.f)
                {
                    IntersectOBBtreeRecursive(right_child_traveler, obb_tree, matrix, best_intersection_so_far_info);
                }
                if (left_paralgram_booleanMinMax.second.first < best_intersection_so_far_info.distanceFromOrigin && left_paralgram_booleanMinMax.second.second >= 0.f)
                {
                    IntersectOBBtreeRecursive(left_child_traveler, obb_tree, matrix, best_intersection_so_far_info);
                }
            }
        }
        else if (left_paralgram_booleanMinMax.first)
        {
            if (left_paralgram_booleanMinMax.second.first < best_intersection_so_far_info.distanceFromOrigin && left_paralgram_booleanMinMax.second.second >= 0.f)
            {
                IntersectOBBtreeRecursive(left_child_traveler, obb_tree, matrix, best_intersection_so_far_info);
            }
        }
        else if (right_paralgram_booleanMinMax.first)
        {
            if (right_paralgram_booleanMinMax.second.first < best_intersection_so_far_info.distanceFromOrigin && right_paralgram_booleanMinMax.second.second >= 0.f)
            {
                IntersectOBBtreeRecursive(right_child_traveler, obb_tree, matrix, best_intersection_so_far_info);
            }
        }
    }
    else
    {
        for (size_t i = obb_tree_traveler.GetTrianglesOffset(); i != obb_tree_traveler.GetTrianglesOffset() + obb_tree_traveler.GetTrianglesCount(); ++i)
        {
            TrianglePosition this_triangle = matrix * obb_tree.GetTrianglePosition(i);
            RayTriangleIntersectInfo interseption_result = IntersectTriangle(this_triangle);

            if (interseption_result.doIntersect &&
                interseption_result.distanceFromOrigin > 0.f &&
                interseption_result.distanceFromOrigin < best_intersection_so_far_info.distanceFromOrigin )
            {
                best_intersection_so_far_info.doIntersect = true;
                best_intersection_so_far_info.itBackfaces = interseption_result.itBackfaces;
                best_intersection_so_far_info.distanceFromOrigin = interseption_result.distanceFromOrigin;
                best_intersection_so_far_info.triangle_index = i;
                best_intersection_so_far_info.baryPosition = interseption_result.baryPosition;
            }             
        }
    }
}