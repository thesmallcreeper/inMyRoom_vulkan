#version 450

#ifdef VERT_POSITION
layout( location = VERT_POSITION_LOCATION ) in vec4 app_position;
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
}
