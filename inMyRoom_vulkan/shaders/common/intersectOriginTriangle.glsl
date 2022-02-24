struct IntersectTriangleResult {
    vec2 barycoords;
    vec2 barycoordsDx;
    vec2 barycoordsDy;

    float distance;
};

IntersectTriangleResult IntersectTriangle(vec3 vert_0, vec3 edge_1, vec3 edge_2,
                                          vec3 dir, vec3 dir_dx, vec3 dir_dy,
                                          vec3 origin, vec3 origin_dx, vec3 origin_dy)
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

    return_struct.barycoordsDx.x = dot(c_x, origin_dx + return_struct.distance * dir_dx);
    return_struct.barycoordsDx.y = dot(c_y, origin_dx + return_struct.distance * dir_dx);
    return_struct.barycoordsDy.x = dot(c_x, origin_dy + return_struct.distance * dir_dy);
    return_struct.barycoordsDy.y = dot(c_y, origin_dy + return_struct.distance * dir_dy);

    return_struct.barycoordsDx *= inv_k;
    return_struct.barycoordsDy *= inv_k;

    return return_struct;
}