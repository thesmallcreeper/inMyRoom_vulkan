#version 450

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


layout( location = 0 ) out vec3 vert_position;


#ifdef VERT_TEXCOORD0
layout( location = 1 ) out vec2 vert_texcoord0;
#endif

#ifdef VERT_TEXCOORD1
layout( location = 2 ) out vec2 vert_texcoord1;
#endif

#ifdef VERT_COLOR0
layout( location = 3 ) out vec4 vert_color0;
#endif

// Push constants (0 byte->63 byte)

layout (push_constant) uniform PushConstants
{
     layout(offset = 0) mat4 TRSmatrix;
};

// Descriptor Sets

layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 projectionMatrix;
};

layout( set = 0 , binding = 1 ) uniform cameraMatrixBuffer
{
    mat4 cameraMatrix;
};


void main()
{
    vec4 position = cameraMatrix * TRSmatrix * app_position;
    
    
    gl_Position = projectionMatrix * position;
    
    vert_position = position.xyz;
    
    #ifdef VERT_TEXCOORD0 
    {
        vert_texcoord0 = vec2(app_texcoord0.x, app_texcoord0.y);
    }
    #endif
    
    #ifdef VERT_TEXCOORD1
    {
        vert_texcoord1 = vec2(app_texcoord1.x, app_texcoord1.y);
    } 
    #endif
    
    #ifdef VERT_COLOR0 
    {
        vert_color0 = app_color0;
    }
    #endif
}
