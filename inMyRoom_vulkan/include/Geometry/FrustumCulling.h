#pragma once
#include <array>

#include "Geometry/Plane.h"
#include "Geometry/Paralgram.h"

class FrustumCulling
{
public:
    void SetFrustumPlanes(std::array<Plane, 6> in_frustum_planes);

    bool IsParalgramInsideFrustum(Paralgram in_paralgram) const;
private:
    std::array<Plane, 6> frustumPlanes;
};

