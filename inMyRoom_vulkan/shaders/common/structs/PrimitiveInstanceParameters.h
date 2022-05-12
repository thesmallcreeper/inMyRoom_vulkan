#ifndef FILE_PRIMITIVE_INSTANCE_PARAMETERS

#include "common/structs/cppInterface.h"

struct PrimitiveInstanceParameters {
    UINT_T indicesOffset;      // uint32_t
    UINT_T positionOffset;     // glm::vec4
    UINT_T normalOffset;       // glm::vec4
    UINT_T tangentOffset;      // glm::vec4
    UINT_T texcoordsOffset;    // glm::vec2
    UINT_T colorOffset;        // glm::vec4

    UINT16_T light;            // != -1 if it is a light
    UINT16_T matricesOffset;
    UINT16_T prevMatricesOffset;
    UINT16_T material;
    UINT16_T lightsCombinationsOffset;
    UINT16_T lightsCombinationsCount;
    UINT16_T positionDescriptorIndex;
    UINT16_T normalDescriptorIndex;
    UINT16_T tangentDescriptorIndex;
    UINT16_T texcoordsDescriptorIndex;
    UINT16_T colorDescriptorIndex;

    UINT8_T indicesSetMultiplier;
    UINT8_T texcoordsStepMultiplier;
    UINT8_T colorStepMultiplier;
};

#define FILE_PRIMITIVE_INSTANCE_PARAMETERS
#endif