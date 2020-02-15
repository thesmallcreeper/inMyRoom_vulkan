#include "CollisionDetection/RayDeltaUncollide.h"
#include "Geometry/Ray.h"

#include <iostream>

RayDeltaUncollide::RayDeltaUncollide(float in_ray_distance_bias_multiplier)
    :ray_distance_bias_multiplier(in_ray_distance_bias_multiplier)
{
}

std::pair<glm::vec3, glm::vec3> RayDeltaUncollide::ExecuteRayDeltaUncollide(const CSentriesPairCollisionCenter& this_entriesPaircollisionCenter) const
{
    glm::vec4 collision_point_worldspace = glm::vec4(this_entriesPaircollisionCenter.collisionPoint, 1.f);

    glm::vec4 first_modelspace_collision_point = glm::inverse(this_entriesPaircollisionCenter.firstEntry.currentGlobalMatrix) * collision_point_worldspace;
    glm::vec4 second_modelspace_collision_point = glm::inverse(this_entriesPaircollisionCenter.secondEntry.currentGlobalMatrix) * collision_point_worldspace;

    glm::vec4 first_previousFrame_collision_point_worldspace = this_entriesPaircollisionCenter.firstEntry.previousGlobalMatrix * first_modelspace_collision_point;
    glm::vec4 second_previousFrame_collision_point_worldspace = this_entriesPaircollisionCenter.secondEntry.previousGlobalMatrix * second_modelspace_collision_point;

    float distance_collision_previousFrame = glm::length(glm::vec3(second_previousFrame_collision_point_worldspace) - glm::vec3(first_previousFrame_collision_point_worldspace));
    float ray_distance_bias = ray_distance_bias_multiplier * distance_collision_previousFrame;

    glm::vec3 first_to_second_previousFrame_direction = glm::normalize(glm::vec3(second_previousFrame_collision_point_worldspace) - glm::vec3(first_previousFrame_collision_point_worldspace));
    glm::vec3 second_to_first_previousFrame_direction = glm::normalize(glm::vec3(first_previousFrame_collision_point_worldspace) - glm::vec3(second_previousFrame_collision_point_worldspace));

    Ray first_to_second_ray = Ray::CreateRay(glm::vec3(collision_point_worldspace) - ray_distance_bias * first_to_second_previousFrame_direction,
                                             first_to_second_previousFrame_direction);

    float best_intersept_distance_first_to_second = std::numeric_limits<float>::infinity();
    Triangle best_triangle_first_to_second;
    InterseptOBBtreeWithRay(reinterpret_cast<const OBBtree*>(this_entriesPaircollisionCenter.secondEntry.OBBtree_ptr),
                            this_entriesPaircollisionCenter.secondEntry.currentGlobalMatrix,
                            first_to_second_ray,
                            best_intersept_distance_first_to_second,
                            best_triangle_first_to_second);

    glm::vec3 first_to_second_normal_found;
    if (best_intersept_distance_first_to_second < std::numeric_limits<float>::infinity())
    {
        Triangle best_triangle_first_to_second_worldspace = this_entriesPaircollisionCenter.secondEntry.currentGlobalMatrix * best_triangle_first_to_second;
        glm::vec3 normal_of_triangle = glm::normalize(glm::cross(glm::vec3(best_triangle_first_to_second_worldspace.GetP1()) - glm::vec3(best_triangle_first_to_second_worldspace.GetP0()),
                                                                 glm::vec3(best_triangle_first_to_second_worldspace.GetP2()) - glm::vec3(best_triangle_first_to_second_worldspace.GetP0())));

        if (glm::dot(normal_of_triangle, second_to_first_previousFrame_direction) < 0.f)
            normal_of_triangle = -normal_of_triangle;

        first_to_second_normal_found = normal_of_triangle;
    }
    else
    {
        best_intersept_distance_first_to_second = distance_collision_previousFrame * 0.6f;
        first_to_second_normal_found = second_to_first_previousFrame_direction;

        std::cout << "Hey!\n";
    }

    Ray second_to_first_ray = Ray::CreateRay(glm::vec3(collision_point_worldspace) - ray_distance_bias * second_to_first_previousFrame_direction,
                                                           second_to_first_previousFrame_direction);

    float best_intersept_distance_second_to_first = std::numeric_limits<float>::infinity();
    Triangle best_triangle_second_to_first;
    InterseptOBBtreeWithRay(reinterpret_cast<const OBBtree*>(this_entriesPaircollisionCenter.firstEntry.OBBtree_ptr),
                            this_entriesPaircollisionCenter.firstEntry.currentGlobalMatrix,
                            second_to_first_ray,
                            best_intersept_distance_second_to_first,
                            best_triangle_second_to_first);

    glm::vec3 second_to_first_normal_found;
    if (best_intersept_distance_second_to_first < std::numeric_limits<float>::infinity())
    {
        Triangle best_triangle_second_to_first_worldspace = this_entriesPaircollisionCenter.firstEntry.currentGlobalMatrix * best_triangle_second_to_first;
        glm::vec3 normal_of_triangle = glm::normalize(glm::cross(glm::vec3(best_triangle_second_to_first_worldspace.GetP1()) - glm::vec3(best_triangle_second_to_first_worldspace.GetP0()),
                                                                 glm::vec3(best_triangle_second_to_first_worldspace.GetP2()) - glm::vec3(best_triangle_second_to_first_worldspace.GetP0())));

        if (glm::dot(normal_of_triangle, first_to_second_previousFrame_direction) < 0.f)
            normal_of_triangle = -normal_of_triangle;

        second_to_first_normal_found = normal_of_triangle;
    }
    else
    {
        best_intersept_distance_second_to_first = distance_collision_previousFrame * 0.6f;
        second_to_first_normal_found = first_to_second_previousFrame_direction;

        std::cout << "Hey!\n";
    }

    float delta = ((2 * ray_distance_bias - best_intersept_distance_first_to_second) + (2 * ray_distance_bias - best_intersept_distance_second_to_first)) - 2 * ray_distance_bias;

    float first_weight = glm::length(this_entriesPaircollisionCenter.collisionPoint - glm::vec3(first_previousFrame_collision_point_worldspace));
    float second_weight = glm::length(this_entriesPaircollisionCenter.collisionPoint - glm::vec3(second_previousFrame_collision_point_worldspace));

    glm::vec3 first_delta = (first_weight / (first_weight + second_weight)) * delta * glm::dot(second_to_first_previousFrame_direction, first_to_second_normal_found) * first_to_second_normal_found;
    glm::vec3 second_delta = (second_weight / (first_weight + second_weight)) * delta * glm::dot(first_to_second_previousFrame_direction, second_to_first_normal_found) * second_to_first_normal_found;

    return std::make_pair(first_delta, second_delta);
}

void RayDeltaUncollide::InterseptOBBtreeWithRay(const OBBtree* OBBtree_ptr,
                                                const glm::mat4x4& matrix,
                                                const Ray& ray,
                                                float& best_intersept_distance,
                                                Triangle& best_triangle_ptr) const
{
    if (!OBBtree_ptr->IsLeaf())
    {
        Cuboid left_cuboid = matrix * OBBtree_ptr->GetLeftChildPtr()->GetOBB();
        Cuboid right_cuboid = matrix * OBBtree_ptr->GetRightChildPtr()->GetOBB();

        std::pair<bool, std::pair<float, float>> left_cuboid_booleanMinMax = Cuboid::IntersectCuboidWithRayBooleanMinMax(left_cuboid, ray);
        std::pair<bool, std::pair<float, float>> right_cuboid_booleanMinMax = Cuboid::IntersectCuboidWithRayBooleanMinMax(right_cuboid, ray);

        if (left_cuboid_booleanMinMax.first && right_cuboid_booleanMinMax.first)
        {
            if (left_cuboid_booleanMinMax.second.first < right_cuboid_booleanMinMax.second.first)
            {
                if (left_cuboid_booleanMinMax.second.first < best_intersept_distance && left_cuboid_booleanMinMax.second.second >= 0.f)
                {
                    InterseptOBBtreeWithRay(OBBtree_ptr->GetLeftChildPtr(), matrix, ray, best_intersept_distance, best_triangle_ptr);
                }
                if (right_cuboid_booleanMinMax.second.first < best_intersept_distance && right_cuboid_booleanMinMax.second.second >= 0.f)
                {
                    InterseptOBBtreeWithRay(OBBtree_ptr->GetRightChildPtr(), matrix, ray, best_intersept_distance, best_triangle_ptr);
                }
            }
            else
            {
                if (right_cuboid_booleanMinMax.second.first < best_intersept_distance && right_cuboid_booleanMinMax.second.second >= 0.f)
                {
                    InterseptOBBtreeWithRay(OBBtree_ptr->GetRightChildPtr(), matrix, ray, best_intersept_distance, best_triangle_ptr);
                }
                if (left_cuboid_booleanMinMax.second.first < best_intersept_distance && left_cuboid_booleanMinMax.second.second >= 0.f)
                {
                    InterseptOBBtreeWithRay(OBBtree_ptr->GetLeftChildPtr(), matrix, ray, best_intersept_distance, best_triangle_ptr);
                }
            }
        }
        else if (left_cuboid_booleanMinMax.first)
        {
            if (left_cuboid_booleanMinMax.second.first < best_intersept_distance && left_cuboid_booleanMinMax.second.second >= 0.f)
            {
                InterseptOBBtreeWithRay(OBBtree_ptr->GetLeftChildPtr(), matrix, ray, best_intersept_distance, best_triangle_ptr);
            }
        }
        else if (right_cuboid_booleanMinMax.first)
        {
            if (right_cuboid_booleanMinMax.second.first < best_intersept_distance && right_cuboid_booleanMinMax.second.second >= 0.f)
            {
                InterseptOBBtreeWithRay(OBBtree_ptr->GetRightChildPtr(), matrix, ray, best_intersept_distance, best_triangle_ptr);
            }
        }
    }
    else
    {
        for (const Triangle& this_triangle_modelspace : OBBtree_ptr->GetTriangles())
        {
            Triangle this_triangle = matrix * this_triangle_modelspace;
            TriangleRayInterseptionInfo interseption_result = Triangle::InterseptTriangleWithRayInfo(this_triangle, ray);

            if (interseption_result.doIntersept &&
                interseption_result.distanceFromOrigin > 0.f &&
                interseption_result.distanceFromOrigin < best_intersept_distance )
            {
                best_intersept_distance = interseption_result.distanceFromOrigin;
                best_triangle_ptr = this_triangle_modelspace;
            }             
        }
    }

}