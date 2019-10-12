#pragma once
#include <array>

#include "Plane.h"
#include "Cuboid.h"

class FrustumCulling
{
public:
    void setFrustumPlanes(const std::array<Plane, 6> in_frustum_planes);

    bool isCuboidInsideFrustum(const Cuboid in_cuboid) const;
private:
    std::array<Plane, 6> frustumPlanes;
};

