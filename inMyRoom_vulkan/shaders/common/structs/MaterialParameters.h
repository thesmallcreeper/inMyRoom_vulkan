#ifndef FILE_MATERIAL_PARAMETERS

#include "common/structs/cppInterface.h"

struct MaterialParameters
{
    VEC4 baseColorFactors;
    UINT_T baseColorTexture;
    UINT_T baseColorTexCoord;

    FLOAT_T alphaCutoff;
};

#define FILE_MATERIAL_PARAMETERS
#endif