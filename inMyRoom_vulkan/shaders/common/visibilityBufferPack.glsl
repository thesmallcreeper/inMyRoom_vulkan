#ifndef FILE_VISIBILITY_BUFFER_PACK

uint PackVisibilityBuffer(uint primitive_id, uint triangle_id)
{
    primitive_id = primitive_id << VISIBILITY_BUFFER_TRIANGLE_BITS;
    return primitive_id | triangle_id;
}

uvec2 UnpackVisibilityBuffer(uint input_packed)
{
    uint primitive_id = input_packed >> VISIBILITY_BUFFER_TRIANGLE_BITS;

    uint triangle_mask = (uint(1) << VISIBILITY_BUFFER_TRIANGLE_BITS) - 1;
    uint triangle_id = input_packed & triangle_mask;

    return uvec2(primitive_id, triangle_id);
}

#define FILE_VISIBILITY_BUFFER_PACK
#endif