#ifndef FILE_AABB

#include "common/structs/cppInterface.h"

struct AABB
{
    VEC3 max_coords;
    VEC3 min_coords;
};

struct AABBintCasted
{
    IVEC3 max_coords_intCasted;
    IVEC3 min_coords_intCasted;
};

#define FILE_AABB
#endif