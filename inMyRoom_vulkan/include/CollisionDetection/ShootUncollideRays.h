#pragma once

#include "CollisionDetection/CreateUncollideRays.h"

class ShootUncollideRays
{
private:
    struct HermannPassResult
    {
        bool successful_ray = false;

        glm::vec3 response;
        glm::vec3 point_objB_normal;
    };

    static HermannPassResult HermannPass(const OBBtree* objA_OBBtree_ptr,
                                         const OBBtree* objB_OBBtree_ptr,
                                         const glm::mat4& objA_mat,
                                         const glm::mat4& objB_mat,
                                         const glm::mat3& objB_normalCorrected_mat,
                                         Ray ray);

    static Ray ReflectHermannResult(const HermannPassResult& hermann_pass_result,
                                    const Ray& ray);

    static glm::vec3 CalcForceResponse(const HermannPassResult& hermann_pass_result);
public:
    ShootUncollideRays(float max_cos_from_force_response_smoothstep_start_rads,
                       float max_cos_from_force_response_smoothstep_finish_rads,
                       float in_ray_distance_bias_multiplier);

    glm::vec3 ExecuteShootUncollideRays(const CDentriesUncollideRays& this_entriesPaircollisionCenter) const;

private:
    glm::vec3 FindResponse(const std::vector<glm::vec3>& ray_responses, const glm::vec3& normalized_force_response) const;

    static float SmootherStep(float edge_a, float edge_b, float x);

    const float max_cos_from_force_response_smoothstep_start;
    const float max_cos_from_force_response_smoothstep_finish;
    const float ray_distance_bias_multiplier = 1.f;

};