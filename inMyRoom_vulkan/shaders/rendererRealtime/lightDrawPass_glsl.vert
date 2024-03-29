#version 460

#include "common/structs/ModelMatrices.h"

#define DIR_INF 1000.f

//
// In
layout( location = 0 ) in vec4 app_position;

//
// Descriptor Sets
layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 viewMatrix;
    mat4 inverseViewMatrix;
    mat4 projectionMatrix;
};

layout( std140, set = 1 , binding = 0 ) readonly buffer matricesBuffer
{
    ModelMatrices model_matrices[MATRICES_COUNT];
};

//
// Push constant
layout (push_constant) uniform PushConstants
{
    layout(offset = 0) uint matrixOffset;
    layout(offset = 4) uint isDirectional;
    layout(offset = 8) uint isSky;
};

//
// Main!
void main()
{
    if (isSky != 0) {
        gl_Position = vec4(app_position.xy, 1.f, 1.f);
    } else {
        mat4 pos_matrix = model_matrices[matrixOffset].positionMatrix;
        if (isDirectional != 0) {
            pos_matrix[0] = pos_matrix[0] * DIR_INF;
            pos_matrix[1] = pos_matrix[1] * DIR_INF;
            pos_matrix[2] = pos_matrix[2] * DIR_INF;
            pos_matrix[3] = vec4(pos_matrix[3].xyz * DIR_INF, 1.f);

            vec4 view_position = pos_matrix * app_position;
            vec4 projection = projectionMatrix * view_position;
            gl_Position = projection.xyww;
        } else {
            vec4 view_position = pos_matrix * app_position;
            vec4 projection = projectionMatrix * view_position;
            gl_Position = projection;
        }
    }
}