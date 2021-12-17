#include "CollisionDetection/ShootUncollideRays.h"
#include <algorithm>

ShootUncollideRays::ShootUncollideRays(float max_cos_from_force_response_smoothstep_start,
                                       float max_cos_from_force_response_smoothstep_finish,
                                       float in_ray_distance_bias_multiplier)
    :
    max_cos_from_force_response_smoothstep_start(std::cos(max_cos_from_force_response_smoothstep_start)),
    max_cos_from_force_response_smoothstep_finish(std::cos(max_cos_from_force_response_smoothstep_finish)),
    ray_distance_bias_multiplier(in_ray_distance_bias_multiplier)
{
}

glm::vec3 ShootUncollideRays::ExecuteShootUncollideRays(const CDentriesUncollideRays& entriesUncollideRaysPair) const
{
    const glm::mat4 second_to_first_space_matrix = glm::inverse(entriesUncollideRaysPair.firstEntry.currentGlobalMatrix) *
                                                   entriesUncollideRaysPair.secondEntry.currentGlobalMatrix;

    const glm::mat3 second_to_first_space_normal_matrix = Triangle::GetNormalCorrectedMatrixUnormalized(second_to_first_space_matrix);

    const OBBtree* first_OBBtree_ptr = entriesUncollideRaysPair.firstEntry.OBBtree_ptr;
    const OBBtree* second_OBBtree_ptr = entriesUncollideRaysPair.secondEntry.OBBtree_ptr;

    glm::vec3 average_force_responses = glm::vec3(0.f);
    std::vector<glm::vec3> ray_responses;

    auto first_to_second_ray_execute = [&](const Ray& ray)
    {
        HermannPassResult pass_result = HermannPass(first_OBBtree_ptr,
                                                    second_OBBtree_ptr,
                                                    glm::mat4(1.f),
                                                    second_to_first_space_matrix,
                                                    second_to_first_space_normal_matrix,
                                                    ray);

        if(pass_result.successful_ray)
        {
            average_force_responses -= CalcForceResponse(pass_result);
            ray_responses.emplace_back(-pass_result.response);

            return std::make_pair(true, ReflectHermannResult(pass_result, ray));
        }
        else
        {
            return std::make_pair(false, Ray());
        }
    };

    auto second_to_first_ray_execute = [&](const Ray& ray)
    {
        HermannPassResult pass_result = HermannPass(second_OBBtree_ptr,
                                                    first_OBBtree_ptr,
                                                    second_to_first_space_matrix,
                                                    glm::mat4(1.f),
                                                    glm::mat3(1.f),
                                                    ray);

        if (pass_result.successful_ray)
        {
            average_force_responses += CalcForceResponse(pass_result);
            ray_responses.emplace_back(pass_result.response);

            return std::make_pair(true, ReflectHermannResult(pass_result, ray));
        }
        else
        {
            return std::make_pair(false, Ray());
        }
    };

    for(const Ray& this_ray : entriesUncollideRaysPair.rays_from_first_to_second)
    {
        std::pair<bool, Ray> ray_1_result = first_to_second_ray_execute(this_ray);
        if(ray_1_result.first)
        {
            second_to_first_ray_execute(ray_1_result.second);
        }
    }

    for (const Ray& this_ray : entriesUncollideRaysPair.rays_from_second_to_first)
    {
        std::pair<bool, Ray> ray_1_result = second_to_first_ray_execute(this_ray);
        if (ray_1_result.first)
        {
            first_to_second_ray_execute(ray_1_result.second);
        }
    }

    if (average_force_responses != glm::vec3(0.f))
    {
        glm::vec3 normalized_average_force_responses = glm::normalize(average_force_responses);

        glm::vec3 localspace_response = ray_distance_bias_multiplier * FindResponse(ray_responses, normalized_average_force_responses);
        return {entriesUncollideRaysPair.firstEntry.currentGlobalMatrix * glm::vec4(localspace_response, 0.f)};
    }
    else
    {
        return glm::vec3(0.f);
    }
}

Ray ShootUncollideRays::ReflectHermannResult(const HermannPassResult& hermann_pass_result, const Ray& ray)
{
    glm::vec3 origin = ray.GetOrigin() + hermann_pass_result.response;
    glm::vec3 direction = - hermann_pass_result.point_objB_normal;

    return {origin, direction};
}

glm::vec3 ShootUncollideRays::CalcForceResponse(const HermannPassResult& hermann_pass_result)
{
    glm::vec3 response_direction = glm::normalize(hermann_pass_result.response);
    float response_length = glm::length(hermann_pass_result.response);

    float cos_angle_with_normal = glm::dot(hermann_pass_result.point_objB_normal, response_direction);
    float cos_angle_with_normal_squared = cos_angle_with_normal * cos_angle_with_normal;

    return response_direction * (cos_angle_with_normal_squared * response_length);
}

ShootUncollideRays::HermannPassResult ShootUncollideRays::HermannPass(const OBBtree* objA_OBBtree_ptr,
                                                                      const OBBtree* objB_OBBtree_ptr,
                                                                      const glm::mat4& objA_mat,
                                                                      const glm::mat4& objB_mat,
                                                                      const glm::mat3& objB_normalCorrected_mat,
                                                                      Ray ray)
{
    ShootUncollideRays::HermannPassResult return_result;

    RayOBBtreeIntersectInfo point2_ray_result = ray.IntersectOBBtree(*objB_OBBtree_ptr, objB_mat);
    if (point2_ray_result.doIntersect && point2_ray_result.itBackfaces)
    {

        glm::vec3 original_ray_origin = ray.GetOrigin();

        ray.MoveOriginEpsilonTowardsDirection(4.f);
        float epsilon_distance = glm::length(ray.GetOrigin() - original_ray_origin);

        RayOBBtreeIntersectInfo point3_ray_result = ray.IntersectOBBtree(*objA_OBBtree_ptr, objA_mat);

        if (point2_ray_result.distanceFromOrigin <= point3_ray_result.distanceFromOrigin + epsilon_distance)
        {
            return_result.successful_ray = true;
            return_result.response = point2_ray_result.distanceFromOrigin * ray.GetDirection();

            TriangleNormal triangle_normal = objB_OBBtree_ptr->GetTriangleNormal(point2_ray_result.triangle_index);
            return_result.point_objB_normal = triangle_normal.GetNormal(point2_ray_result.baryPosition, objB_normalCorrected_mat);
        }
    }

    return return_result;
}

glm::vec3 ShootUncollideRays::FindResponse(const std::vector<glm::vec3>& ray_responses, const glm::vec3& normalized_force_response) const
{
    float max_length = 0.f;

    for(const glm::vec3& this_ray_response: ray_responses)
    {
        glm::vec3 this_normalized_ray_response = glm::normalize(this_ray_response);
        float dot_angle_with_force = glm::dot(this_normalized_ray_response, normalized_force_response);

        float this_ray_length = glm::length(this_ray_response);
        float needed_force_response_length = this_ray_length / dot_angle_with_force;

        float smoothstep_filter = SmootherStep(max_cos_from_force_response_smoothstep_finish,
                                               max_cos_from_force_response_smoothstep_start,
                                               dot_angle_with_force);

        max_length = std::max(max_length, smoothstep_filter * needed_force_response_length);
    }

    return max_length * normalized_force_response;
}

float ShootUncollideRays::SmootherStep(float edge_a, float edge_b, float x)
{
    float tmp = std::clamp((x - edge_a) / (edge_b - edge_a), 0.f, 1.f);
    return tmp * tmp * tmp * (tmp * (tmp * 6 - 15) + 10);
}