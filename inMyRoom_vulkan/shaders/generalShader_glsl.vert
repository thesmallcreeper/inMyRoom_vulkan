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

#ifdef VERT_JOINTS0
layout( location = VERT_JOINTS0_LOCATION ) in uvec4 app_joints0;
#endif

#ifdef VERT_WEIGHTS0
layout( location = VERT_WEIGHTS0_LOCATION ) in vec4 app_weights0;
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

#ifndef USE_SKIN
layout (push_constant) uniform PushConstants
{
     layout(offset = 0) mat4 TRSmatrix;
};
#else
layout (push_constant) uniform PushConstants
{
     layout(offset = 0) uint inverseBindMatricesOffset;
	 layout(offset = 4) uint nodesMatricesOffset;
};
#endif

// Descriptor Sets

layout( set = 0 , binding = 0 ) uniform projectionMatrixBuffer
{
    mat4 projectionMatrix;
};
layout( set = 0 , binding = 1 ) uniform cameraMatrixBuffer
{
    mat4 cameraMatrix;
};

#ifdef USE_SKIN
layout( set = 1 , binding = 0 ) uniform inverseMatricesBuffer
{
    mat4 inverseMatrices[INVERSE_BIND_COUNT];
};
layout( set = 1 , binding = 1 ) uniform nodesMatricesBuffer
{
    mat4 nodesMatrices[NODES_MATRICES_COUNT];
};
#endif


void main()
{
	#ifndef USE_SKIN
    vec4 world_position = TRSmatrix * app_position;
    #else
	mat4 skin_matrix = mat4(0.f);
	
	skin_matrix += app_weights0.x * nodesMatrices[app_joints0.x+nodesMatricesOffset] * inverseMatrices[app_joints0.x+inverseBindMatricesOffset]; 
	skin_matrix += app_weights0.y * nodesMatrices[app_joints0.y+nodesMatricesOffset] * inverseMatrices[app_joints0.y+inverseBindMatricesOffset]; 
	skin_matrix += app_weights0.z * nodesMatrices[app_joints0.z+nodesMatricesOffset] * inverseMatrices[app_joints0.z+inverseBindMatricesOffset];
	skin_matrix += app_weights0.w * nodesMatrices[app_joints0.w+nodesMatricesOffset] * inverseMatrices[app_joints0.w+inverseBindMatricesOffset];
	
	vec4 world_position = skin_matrix * app_position;
	#endif
	
	vec4 camera_position = cameraMatrix * world_position;
	
    gl_Position = projectionMatrix * camera_position;    
    vert_position = camera_position.xyz;
    
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
