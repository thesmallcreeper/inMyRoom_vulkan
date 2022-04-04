
// From: A fast and robust method for avoiding self-intersection
// Ray tracing gems 2019

#define RAY_OFFSET_ORIGIN (1.f / 32.f)
#define RAY_OFFSET_FLOAT_SCALE (1.f / 65536.f)
#define RAY_OFFSET_INT_SCALE (256.f)

vec3 RayOffsetFace(vec3 p, vec3 n)
{
    ivec3 of_i = ivec3(RAY_OFFSET_INT_SCALE * n.x, RAY_OFFSET_INT_SCALE * n.y, RAY_OFFSET_INT_SCALE * n.z);

    vec3 p_i = vec3(intBitsToFloat(floatBitsToInt(p.x) + ((p.x < 0) ? -of_i.x : +of_i.x)),
                    intBitsToFloat(floatBitsToInt(p.y) + ((p.y < 0) ? -of_i.y : +of_i.y)),
                    intBitsToFloat(floatBitsToInt(p.z) + ((p.z < 0) ? -of_i.z : +of_i.z)));

    vec3 result = vec3((abs(p.x) < RAY_OFFSET_ORIGIN) ? p.x + RAY_OFFSET_FLOAT_SCALE * n.x : p_i.x,
                       (abs(p.y) < RAY_OFFSET_ORIGIN) ? p.y + RAY_OFFSET_FLOAT_SCALE * n.y : p_i.y,
                       (abs(p.z) < RAY_OFFSET_ORIGIN) ? p.z + RAY_OFFSET_FLOAT_SCALE * n.z : p_i.z);
    return result;
}