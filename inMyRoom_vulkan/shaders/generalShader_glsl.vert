#version 450

#ifdef VERT_POSITION
layout( location = VERT_POSITION_LOCATION ) in vec4 app_position;
#endif

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




layout( location = 0 ) out vec3 vert_position;

#ifdef VERT_TEXCOORD0
layout( location = 1 ) out vec2 vert_texcoord;
#endif


layout(std140, set = 0 , binding = 0) restrict readonly buffer StorageBuffer0
{
    mat4 GlobalTRSMatrixes[N_MESHIDS];
};

layout( set = 1 , binding = 0 ) uniform UniformBuffer0
{
    mat4 ProjectionMatrix;
};

layout( set = 1 , binding = 1 ) uniform UniformBuffer1
{
    mat4 CameraMatrix;
};


void main()
{
    const int thisIndex = gl_InstanceIndex;
    const mat4 thisGlobalTRSMatrix = GlobalTRSMatrixes[thisIndex];

	vec4 position = CameraMatrix * thisGlobalTRSMatrix * app_position;
	
	
    gl_Position = ProjectionMatrix * position;	
	
	vert_position = position.xyz;
	
	#ifdef VERT_TEXCOORD0
	vert_texcoord = vec2(app_texcoord0.x, app_texcoord0.y);
	#endif
}
