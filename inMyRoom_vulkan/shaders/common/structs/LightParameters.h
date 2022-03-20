#ifndef FILE_LIGHT_PARAMETERS

#include "common/structs/cppInterface.h"

struct LightParameters {
    VEC3 luminance;
    FLOAT_T radius;
    FLOAT_T length;
    FLOAT_T range;

    UINT16_T matricesOffset;
    UINT8_T lightType;
};

#define FILE_LIGHT_PARAMETERS
#endif