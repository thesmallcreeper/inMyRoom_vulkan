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

    RayOBBtreeIntersectInfo first_to_second_intersect = first_to_second_ray.IntersectOBBtree(*reinterpret_cast<const OBBtree*>(this_entriesPaircollisionCenter.secondEntry.OBBtree_ptr),
                                                                                              this_entriesPaircollisionCenter.secondEntry.currentGlobalMatrix);

    float best_intersept_distance_first_to_second = first_to_second_intersect.distanceFromOrigin;
    glm::vec3 first_to_second_normal_found;
    if (first_to_second_intersect.doIntersect)
    {
        TrianglePosition best_triangle_first_to_second_worldspace = this_entriesPaircollisionCenter.secondEntry.currentGlobalMatrix * 
            reinterpret_cast<const OBBtree*>(this_entriesPaircollisionCenter.secondEntry.OBBtree_ptr)->GetTrianglePosition(first_to_second_intersect.triangle_index);
        glm::vec3 normal_of_triangle = glm::normalize(glm::cross(glm::vec3(best_triangle_first_to_second_worldspace.GetP1()) - glm::vec3(best_triangle_first_to_second_worldspace.GetP0()),
                                                                 glm::vec3(best_triangle_first_to_second_worldspace.GetP2()) - glm::vec3(best_triangle_first_to_second_worldspace.GetP0())));

        if (glm::dot(normal_of_triangle, second_to_first_previousFrame_direction) < 0.f)
            normal_of_triangle = -normal_of_triangle;

        first_to_second_normal_found = normal_of_triangle;
    }
    else
    {
        best_intersept_distance_first_to_second = ray_distance_bias * 0.9f;
        first_to_second_normal_found = second_to_first_previousFrame_direction;
    }

    Ray second_to_first_ray = Ray::CreateRay(glm::vec3(collision_point_worldspace) - ray_distance_bias * second_to_first_previousFrame_direction,
                                                           second_to_first_previousFrame_direction);

    RayOBBtreeIntersectInfo second_to_first_intersect = second_to_first_ray.IntersectOBBtree(*reinterpret_cast<const OBBtree*>(this_entriesPaircollisionCenter.firstEntry.OBBtree_ptr),
                                                                                              this_entriesPaircollisionCenter.firstEntry.currentGlobalMatrix);

    float best_intersept_distance_second_to_first = second_to_first_intersect.distanceFromOrigin;
    glm::vec3 second_to_first_normal_found;
    if (second_to_first_intersect.doIntersect)
    {
        TrianglePosition best_triangle_second_to_first_worldspace = this_entriesPaircollisionCenter.firstEntry.currentGlobalMatrix *
            reinterpret_cast<const OBBtree*>(this_entriesPaircollisionCenter.firstEntry.OBBtree_ptr)->GetTrianglePosition(second_to_first_intersect.triangle_index);
        glm::vec3 normal_of_triangle = glm::normalize(glm::cross(glm::vec3(best_triangle_second_to_first_worldspace.GetP1()) - glm::vec3(best_triangle_second_to_first_worldspace.GetP0()),
                                                                 glm::vec3(best_triangle_second_to_first_worldspace.GetP2()) - glm::vec3(best_triangle_second_to_first_worldspace.GetP0())));

        if (glm::dot(normal_of_triangle, first_to_second_previousFrame_direction) < 0.f)
            normal_of_triangle = -normal_of_triangle;

        second_to_first_normal_found = normal_of_triangle;
    }
    else
    {
        best_intersept_distance_second_to_first = ray_distance_bias * 0.9f;
        second_to_first_normal_found = first_to_second_previousFrame_direction;
    }

    float delta = ((2 * ray_distance_bias - best_intersept_distance_first_to_second) + (2 * ray_distance_bias - best_intersept_distance_second_to_first)) - 2 * ray_distance_bias;

    float first_weight = glm::length(this_entriesPaircollisionCenter.collisionPoint - glm::vec3(first_previousFrame_collision_point_worldspace));
    float second_weight = glm::length(this_entriesPaircollisionCenter.collisionPoint - glm::vec3(second_previousFrame_collision_point_worldspace));

    glm::vec3 first_delta = (first_weight / (first_weight + second_weight)) * delta * glm::dot(second_to_first_previousFrame_direction, first_to_second_normal_found) * first_to_second_normal_found;
    glm::vec3 second_delta = (second_weight / (first_weight + second_weight)) * delta * glm::dot(first_to_second_previousFrame_direction, second_to_first_normal_found) * second_to_first_normal_found;

    return std::make_pair(first_delta, second_delta);
}
