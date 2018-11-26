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

void main()
{
	gl_Position = ProjectionMatrix * CameraMatrix * app_position;
}