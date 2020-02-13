#include "Geometry/Ray.h"

Ray Ray::CreateRay(const glm::vec3 in_origin, const glm::vec3 in_direction)
{
    Ray return_ray;

    return_ray.origin = glm::vec4(in_origin, 1.f);
    return_ray.direction = glm::vec4(in_direction, 0.f);

    return return_ray;
}

glm::vec4 Ray::GetOrigin() const
{
    return origin;
}

glm::vec4 Ray::GetDirection() const
{
    return direction;
}
