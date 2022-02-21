#version 460

//
// Out
layout( location = 0 ) out vec4 color_out;


/// 0, 0
layout (input_attachment_index = 0, set = INPUT_ATTACHMENT_SET, binding = INPUT_ATTACHMENT_BIND) uniform subpassInput colorInput;

//
// Push constants
layout (push_constant) uniform PushConstants
{
    layout(offset = 0)     float scale;
};

void main()
{
    vec4 color = vec4(vec3(subpassLoad(colorInput)) * scale, 1.f);
    color_out = color;
}