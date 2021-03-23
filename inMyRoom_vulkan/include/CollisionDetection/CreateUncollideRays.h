#pragma once

#include "CollisionDetection/OBBtreesCollision.h"
#include "Geometry/Ray.h"

struct CDentriesUncollideRays
{
    CollisionDetectionEntry firstEntry;
    CollisionDetectionEntry secondEntry;
    std::vector<Ray> rays_from_first_to_second;
    glm::vec3 average_point_first_modelspace;
    std::vector<Ray> rays_from_second_to_first;
    glm::vec3 average_point_second_modelspace;
};

class CreateUncollideRays
{
public:
    CreateUncollideRays();

    // Intersections are happening at first entry space
    CDentriesUncollideRays ExecuteCreateUncollideRays(const CDentriesPairTrianglesPairs& CSentriesPairTrianglesPairs) const;
};