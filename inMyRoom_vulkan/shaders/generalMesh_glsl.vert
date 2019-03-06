#version 430

layout( location = 0 ) in vec4 app_position;

layout( set = 0 , binding = 0 ) uniform UniformBuffer0
{
	mat4 ProjectionMatrix;
};

layout( set = 0 , binding = 1 ) uniform UniformBuffer1
{
	mat4 CameraMatrix;
};

layout(std140, set = 0 , binding = 2) restrict readonly buffer StorageBuffer0
{
	mat4 GlobalTRSMatrixes[N_MESHIDS];
};

void main()
{
	const int thisIndex = gl_InstanceIndex;
	const mat4 thisGlobalTRSMatrix = GlobalTRSMatrixes[thisIndex];

	gl_Position = ProjectionMatrix * CameraMatrix * thisGlobalTRSMatrix * app_position;
}