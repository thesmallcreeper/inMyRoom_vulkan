#version 450

layout( location = 0 ) in vec4 app_position;


layout (push_constant) uniform PC
{
    mat4 TRSMatrix;
};

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
    vec4 position = CameraMatrix * TRSMatrix * app_position;
    gl_Position = ProjectionMatrix * position;
}
