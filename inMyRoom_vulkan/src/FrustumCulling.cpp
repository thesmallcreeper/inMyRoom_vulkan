#include "FrustumCulling.h"

void FrustumCulling::setFrustumPlanes(const std::array<Plane, 6> in_frustum_planes)
{
    frustumPlanes = in_frustum_planes;
}

bool FrustumCulling::isCuboidInsideFrustum(const Cuboid in_cuboid) const
{
    for (size_t i = 0; i < 6; i++)
    {
        if (frustumPlanes[i].intersectCuboid(in_cuboid) == OUTSIDE)
            return false;
    }

    return true;
}
