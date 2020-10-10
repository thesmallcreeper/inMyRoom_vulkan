#pragma once
#include <array>

#include "Geometry/Plane.h"
#include "Geometry/Cuboid.h"

class FrustumCulling
{
public:
    void SetFrustumPlanes(std::array<Plane, 6> in_frustum_planes);

    bool IsCuboidInsideFrustum(Cuboid in_cuboid) const;
private:
    std::array<Plane, 6> frustumPlanes;
};

