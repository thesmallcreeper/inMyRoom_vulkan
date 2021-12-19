#version 450

// In

layout( location = 0 ) in vec4 app_position;

#ifdef VERT_NORMAL
layout( location = VERT_NORMAL_LOCATION ) in vec4 app_normal;
#endif

#ifdef VERT_TANGENT
layout( location = VERT_TANGENT_LOCATION ) in vec4 app_tangent;
#endif

#ifdef VERT_TEXCOORD0
layout( location = VERT_TEXCOORD0_LOCATION ) in vec2 app_texcoord0;
#endif

#ifdef VERT_TEXCOORD1
layout( location = VERT_TEXCOORD1_LOCATION ) in vec2 app_texcoord1;
#endif

#ifdef VERT_COLOR0
layout( location = VERT_COLOR0_LOCATION ) in vec4 app_color0;
#endif

#ifdef VERT_JOINTS0
layout( location = VERT_JOINTS0_LOCATION ) in uvec4 app_joints0;
#endif

#ifdef VERT_WEIGHTS0
layout( location = VERT_WEIGHTS0_LOCATION ) in vec4 app_weights0;
#endif

// Out

#ifdef VERT_TEXCOORD0
layout( location = 0 ) out vec2 vert_texcoord0;
#endif

#ifdef VERT_COLOR0
layout( location = 1 ) out vec4 vert_color0;
#endif

// Push constant

layout (push_constant) uniform PushConstants
{
     layout(offset = 0) uint matrixOffset;
     layout(offset = 4) uint inverseMatricesOffset;
};

// Descriptor Sets

layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 viewMatrix;
    mat4 projectionMatrix;
};

layout( std140, set = 1 , binding = 0 ) readonly buffer projectionMatrixBuffer
{
    mat4 matrices[MATRICES_COUNT];
};

#ifdef USE_SKIN
layout( std140, set = 2 , binding = 0 ) readonly buffer inverseMatricesBuffer
{
    mat4 inverseMatrices[INVERSE_MATRICES_COUNT];
};
#endif


void main()
{
	#ifndef USE_SKIN
    vec4 world_position = matrices[matrixOffset] * app_position;
    #else
	mat4 skin_matrix = mat4(0.f);
	
	skin_matrix += app_weights0.x * matrices[app_joints0.x+matrixOffset+1] * inverseMatrices[app_joints0.x+inverseMatricesOffset];
	skin_matrix += app_weights0.y * matrices[app_joints0.y+matrixOffset+1] * inverseMatrices[app_joints0.y+inverseMatricesOffset];
	skin_matrix += app_weights0.z * matrices[app_joints0.z+matrixOffset+1] * inverseMatrices[app_joints0.z+inverseMatricesOffset];
	skin_matrix += app_weights0.w * matrices[app_joints0.w+matrixOffset+1] * inverseMatrices[app_joints0.w+inverseMatricesOffset];
	
	vec4 world_position = matrices[matrixOffset]*skin_matrix * app_position;
	#endif

    vec4 view_position = viewMatrix * world_position;
    gl_Position = projectionMatrix * view_position;

    #ifdef VERT_TEXCOORD0 
    {
        vert_texcoord0 = vec2(app_texcoord0.x, app_texcoord0.y);
    }
    #endif

    #ifdef VERT_COLOR0 
    {
        vert_color0 = app_color0;
    }
    #endif
}
