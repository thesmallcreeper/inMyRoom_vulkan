#version 460

#include "common/structs/ModelMatrices.h"

//
// In
layout( location = 0 ) in vec4 app_position;

#ifdef IS_MASKED
layout( location = 1 ) in vec2 app_texcoord;
#endif

//
// Out
#ifdef IS_MASKED
layout( location = 0 ) out vec2 vert_texcoord;
#endif

//
// Descriptor Sets
layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
};

layout( std430, set = 1 , binding = 0 ) readonly buffer matricesBuffer
{
    ModelMatrices model_matrices[MATRICES_COUNT];
};

//
// Push constant
layout (push_constant) uniform PushConstants
{
    layout(offset = 0) uint matrixOffset;
};

//
// Main!
void main()
{
    vec4 view_position = model_matrices[matrixOffset].positionMatrix * app_position;

    gl_Position = projectionMatrix * view_position;

    #ifdef IS_MASKED
        vert_texcoord = app_texcoord;
    #endif
}