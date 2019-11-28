#include "Geometry/FrustumCulling.h"

void FrustumCulling::SetFrustumPlanes(const std::array<Plane, 6> in_frustum_planes)
{
    frustumPlanes = in_frustum_planes;
}

bool FrustumCulling::IsCuboidInsideFrustum(const Cuboid in_cuboid) const
{
    for (size_t i = 0; i < 6; i++)
    {
        if (frustumPlanes[i].IntersectCuboid(in_cuboid) == OUTSIDE)
            return false;
    }

    return true;
}
