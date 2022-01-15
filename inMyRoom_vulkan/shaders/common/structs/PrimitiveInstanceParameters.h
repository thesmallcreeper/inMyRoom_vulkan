#ifndef FILE_PRIMITIVE_INSTANCE_PARAMETERS

#include "common/structs/cppInterface.h"

struct PrimitiveInstanceParameters {
    UINT indicesOffset;      // uint32_t
    UINT positionOffset;     // glm::vec4
    UINT normalOffset;       // glm::vec4
    UINT tangentOffset;      // glm::vec4
    UINT texcoordsOffset;    // glm::vec2
    UINT colorOffset;        // glm::vec4

    UINT16_T matricesOffset;
    UINT16_T material;
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