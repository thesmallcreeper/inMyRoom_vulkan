#include "Geometry/FrustumCulling.h"

void FrustumCulling::SetFrustumPlanes(const std::array<Plane, 6> in_frustum_planes)
{
    frustumPlanes = in_frustum_planes;
}

bool FrustumCulling::IsParalgramInsideFrustum(const Paralgram in_paralgram) const
{
    for (size_t i = 0; i < 6; i++)
    {
        if (frustumPlanes[i].IntersectParalgram(in_paralgram) == PlaneIntersectResult::OUTSIDE)
            return false;
    }

    return true;
}
