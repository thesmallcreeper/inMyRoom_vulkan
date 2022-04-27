struct IntersectTriangleResult {
    vec2 barycoords;
    float distance;
};

IntersectTriangleResult IntersectTriangle(vec3 vert_0, vec3 edge_1, vec3 edge_2,
                                          vec3 dir, vec3 origin)
{
    IntersectTriangleResult return_struct;

    vec3 n = cross(edge_1, edge_2);
    vec3 c_x = cross(edge_2, dir);
    vec3 c_y = cross(dir, edge_1);

    vec3 dist = origin - vert_0;
    return_struct.distance = dot(-dist, n);
    return_struct.barycoords.x = dot(dist, c_x);
    return_struct.barycoords.y = dot(dist, c_y);

    float inv_k = 1.f / (dot(n, dir));
    return_struct.distance *= inv_k;
    return_struct.barycoords *= inv_k;

    return return_struct;
}

struct RayDiffsOrigin {
    vec3 originDx;
    vec3 originDy;
};

struct RayDiffsDir {
    vec3 dirDx;
    vec3 dirDy;
};

// Ray gems 2 p.93
RayDiffsOrigin PropagateRayDiffsOrigin(RayDiffsOrigin origin_diffs, RayDiffsDir dir_diffs,
                                       vec3 normal, vec3 dir, float t)
{
    RayDiffsOrigin return_struct;
    return_struct.originDx = origin_diffs.originDx + t * dir_diffs.dirDx;
    return_struct.originDy = origin_diffs.originDy + t * dir_diffs.dirDy;
    float rcpDN = 1.f / dot(normal, dir);
    float dtdx = -dot(return_struct.originDx, normal) * rcpDN;
    float dtdy = -dot(return_struct.originDy, normal) * rcpDN;
    return_struct.originDx += dir * dtdx;
    return_struct.originDy += dir * dtdy;

    return return_struct;
}


RayDiffsDir ReflectRayDiffsDir(RayDiffsDir dir_diffs, vec3 n)
{
    RayDiffsDir return_struct;
    return_struct.dirDx = dir_diffs.dirDx - 2.f * dot(dir_diffs.dirDx, n) * n;
    return_struct.dirDy = dir_diffs.dirDy - 2.f * dot(dir_diffs.dirDy, n) * n;

    return return_struct;
}

struct BarycoordsDiffs {
    vec2 barycoordsDx;
    vec2 barycoordsDy;
};

BarycoordsDiffs ComputeBaryDiffs(RayDiffsOrigin ray_originDiffs,
                                 vec3 edge_1, vec3 edge_2, vec3 faceNormal)
{
    vec3 Nu = cross(edge_2, faceNormal);
    vec3 Nv = cross(edge_1, faceNormal);
    vec3 Lu = Nu / dot(Nu, edge_1);
    vec3 Lv = Nv / dot(Nv, edge_2);

    BarycoordsDiffs return_struct;
    return_struct.barycoordsDx.x = dot(Lu, ray_originDiffs.originDx);
    return_struct.barycoordsDx.y = dot(Lv, ray_originDiffs.originDx);
    return_struct.barycoordsDy.x = dot(Lu, ray_originDiffs.originDy);
    return_struct.barycoordsDy.y = dot(Lv, ray_originDiffs.originDy);

    return return_struct;
}

