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


void main()
{
    vec4 world_position = matrices[matrixOffset] * app_position;

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
