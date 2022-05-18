#version 460

//
// In
layout( location = 0 ) in vec4 app_position;
layout( location = 1 ) in vec4 app_normal;

//
// Out
layout( location = 0 ) out vec3 vert_normal;

//
// Push constant

//
// Descriptor Sets

//
// Main!
void main()
{
    gl_Position = app_position;
    vert_normal = app_normal.xyz;
}