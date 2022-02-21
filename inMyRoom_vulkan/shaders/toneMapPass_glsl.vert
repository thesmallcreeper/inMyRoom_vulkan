#version 460

//
// In
layout( location = 0 ) in vec4 app_position;

//
// Out

//
// Push constant

//
// Descriptor Sets

//
// Main!
void main()
{
    gl_Position = app_position;
}