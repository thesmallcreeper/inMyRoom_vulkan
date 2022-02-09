struct IntersectOriginTriangleResult {
    vec2 barycoords;
    vec2 barycoordsDx;
    vec2 barycoordsDy;

    float distance;
};

IntersectOriginTriangleResult IntersectOriginTriangle(vec3 vert_0, vec3 edge_1, vec3 edge_2, vec3 dir)
{
    IntersectOriginTriangleResult return_struct;

    vec3 dir_dx = dFdx(dir);
    vec3 dir_dy = dFdy(dir);

    vec3 n = cross(edge_1, edge_2);
    vec3 c_x = cross(edge_2, dir);
    vec3 c_y = cross(dir, edge_1);

    return_struct.distance = dot(vert_0, n);
    return_struct.barycoords.x = dot(-vert_0, c_x);
    return_struct.barycoords.y = dot(-vert_0, c_y);
    return_struct.barycoordsDx.x = dot(c_x, dir_dx);
    return_struct.barycoordsDx.y = dot(c_y, dir_dx);
    return_struct.barycoordsDy.x = dot(c_x, dir_dy);
    return_struct.barycoordsDy.y = dot(c_y, dir_dy);

    float inv_k = 1.f / (dot(n, dir));
    return_struct.distance *= inv_k;
    return_struct.barycoords *= inv_k;
    return_struct.barycoordsDx *= inv_k * return_struct.distance;
    return_struct.barycoordsDy *= inv_k * return_struct.distance;

    return return_struct;
}