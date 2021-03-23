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
    // Code based on Realtime Rendering (RTR) 4th edition p. 960

    float min_distance = -std::numeric_limits<float>::infinity();
    float max_distance = +std::numeric_limits<float>::infinity();

    glm::vec3 _P = paralgram.GetCenter() - origin;

    {  // 1. U test
        float e = glm::dot(glm::normalize(paralgram.GetSideDirectionU()), _P);
        float f = glm::dot(glm::normalize(paralgram.GetSideDirectionU()), direction);

        if (std::abs(f) > std::numeric_limits<float>::epsilon())
        {
            float t1 = (e + glm::length(paralgram.GetSideDirectionU())) / f;
            float t2 = (e - glm::length(paralgram.GetSideDirectionU())) / f;
            if (t1 > t2) std::swap(t1, t2);

            if (t1 > min_distance) min_distance = t1;
            if (t2 < max_distance) max_distance = t2;

            if(min_distance > max_distance || max_distance < 0.f)
                return std::make_pair(false, std::make_pair(0.f, 0.f));
        }
        else if (-e - glm::length(paralgram.GetSideDirectionU()) > 0.f || -e + glm::length(paralgram.GetSideDirectionU()) < 0.f)
            return std::make_pair(false, std::make_pair(0.f, 0.f));
    }

    {  // 2. V test
        float e = glm::dot(glm::normalize(paralgram.GetSideDirectionV()), _P);
        float f = glm::dot(glm::normalize(paralgram.GetSideDirectionV()), direction);

        if (std::abs(f) > std::numeric_limits<float>::epsilon())
        {
            float t1 = (e + glm::length(paralgram.GetSideDirectionV())) / f;
            float t2 = (e - glm::length(paralgram.GetSideDirectionV())) / f;
            if (t1 > t2) std::swap(t1, t2);

            if (t1 > min_distance) min_distance = t1;
            if (t2 < max_distance) max_distance = t2;

            if (min_distance > max_distance || max_distance < 0.f)
                return std::make_pair(false, std::make_pair(0.f, 0.f));
        }
        else if (-e - glm::length(paralgram.GetSideDirectionV()) > 0.f || -e + glm::length(paralgram.GetSideDirectionV()) < 0.f)
            return std::make_pair(false, std::make_pair(0.f, 0.f));
    }

    {  // 3. W test
        float e = glm::dot(glm::normalize(paralgram.GetSideDirectionW()), _P);
        float f = glm::dot(glm::normalize(paralgram.GetSideDirectionW()), direction);

        if (std::abs(f) > std::numeric_limits<float>::epsilon())
        {
            float t1 = (e + glm::length(paralgram.GetSideDirectionW())) / f;
            float t2 = (e - glm::length(paralgram.GetSideDirectionW())) / f;
            if (t1 > t2) std::swap(t1, t2);

            if (t1 > min_distance) min_distance = t1;
            if (t2 < max_distance) max_distance = t2;

            if (min_distance > max_distance || max_distance < 0.f)
                return std::make_pair(false, std::make_pair(0.f, 0.f));
        }
        else if (-e - glm::length(paralgram.GetSideDirectionW()) > 0.f || -e + glm::length(paralgram.GetSideDirectionW()) < 0.f)
            return std::make_pair(false, std::make_pair(0.f, 0.f));
    }

    return std::make_pair(true, std::make_pair(min_distance, max_distance));
}

RayOBBtreeIntersectInfo Ray::IntersectOBBtree(const OBBtree& obb_tree, const glm::mat4x4& matrix) const
{
    RayOBBtreeIntersectInfo return_intersect_info;

    OBBtree::OBBtreeTraveler root_traveler = obb_tree.GetRootTraveler();

    Paralgram root_paralgram = matrix * root_traveler.GetOBB();
    std::pair<bool, std::pair<float, float>> root_paralgram_intersect = IntersectParalgram(root_paralgram);

    if(root_paralgram_intersect.first == true && root_paralgram_intersect.second.second >= 0.f)
    {
        IntersectOBBtreeRecursive(root_traveler, obb_tree, matrix, return_intersect_info);
    }
   
    return return_intersect_info;
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