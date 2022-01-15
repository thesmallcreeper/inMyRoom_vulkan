#ifndef FILE_MATERIAL_PARAMETERS

#include "common/structs/cppInterface.h"

struct MaterialParameters
{
    VEC4 baseColorFactors;
    UINT baseColorTexture;
    UINT baseColorTexCoord;

    FLOAT alphaCutoff;
};

#define FILE_MATERIAL_PARAMETERS
#endif