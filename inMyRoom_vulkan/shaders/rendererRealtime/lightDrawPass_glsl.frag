#version 460

#extension GL_EXT_post_depth_coverage : require

//
// In
layout(early_fragment_tests) in;
layout(post_depth_coverage) in;

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
    #ifdef MORPHOLOGICAL_MSAA
    if (luminance.w == 0.f && gl_SampleMaskIn[0] != ((uint(1) << MORPHOLOGICAL_MSAA) - 1))
        alpha = 0.f;
    #endif
    color_out = vec4(luminance.xyz, alpha);
}