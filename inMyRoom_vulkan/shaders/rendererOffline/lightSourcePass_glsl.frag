#version 460

//
// Out
layout( location = 0 ) out vec4 color_out;

//
// Push constants
layout (push_constant) uniform PushConstants
{
    layout(offset = 16) vec4 luminance;
};

//
// Main!

void main()
{
    float alpha = 1.f;
    color_out = vec4(luminance.xyz, alpha);
}