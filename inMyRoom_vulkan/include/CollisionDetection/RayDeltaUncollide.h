#pragma once

#include "CollisionDetection/TrianglesVsTriangles.h"

class RayDeltaUncollide
{
public:
    RayDeltaUncollide(float in_ray_distance_bias_multiplier);

    std::pair<glm::vec3, glm::vec3> ExecuteRayDeltaUncollide(const CSentriesPairCollisionCenter& this_entriesPaircollisionCenter) const;

private:
    void InterseptOBBtreeWithRay(const OBBtree* OBBtree_ptr,
                                 const glm::mat4x4& matrix,
                                 const Ray& ray,
                                 float& best_intersept_distance,
                                 Triangle& best_triangle) const;

    const float ray_distance_bias_multiplier;
};