#pragma once

#include "CollisionDetection/TrianglesVsTriangles.h"

class RayDeltaUncollide
{
public:
    RayDeltaUncollide(float in_ray_distance_bias_multiplier);

    std::pair<glm::vec3, glm::vec3> ExecuteRayDeltaUncollide(const CSentriesPairCollisionCenter& this_entriesPaircollisionCenter) const;

private:
    const float ray_distance_bias_multiplier;
};