#ifndef FILE_MATERIAL_PARAMETERS

#include "common/structs/cppInterface.h"

struct MaterialParameters
{
    VEC4 baseColorFactors;
    UINT_T baseColorTexture;
    UINT_T baseColorTexCoord;

    UINT_T normalTexture;
    UINT_T normalTexCoord;

    UINT_T metallicRoughnessTexture;
    UINT_T metallicRoughnessTexCoord;

    FLOAT_T alphaCutoff;
    FLOAT_T normalScale;
    FLOAT_T metallicFactor;
    FLOAT_T roughnessFactor;
};

#define FILE_MATERIAL_PARAMETERS
#endif