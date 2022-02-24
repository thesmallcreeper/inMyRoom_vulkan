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

// TODO: Credits
float ExpCompress(float val) {
    return 1.f - exp(-val);
}

float RangeCompress(float val, float threshold) {
    float v1 = val;
    float v2 = threshold + (1.f - threshold)*ExpCompress((val - threshold) / (1.f - threshold));
    return val < threshold ? v1 : v2;
}

void main()
{
    float exposure = 1.f / 1.5e3f;
    float linearThreshold = 0.25;

    vec3 color_exposure = vec3(subpassLoad(colorInput)) * scale * exposure;

    float maxColor = max(color_exposure.x, max(color_exposure.y, color_exposure.z));
    float mappedMax = RangeCompress(maxColor, linearThreshold);
    vec3 color_compressed = color_exposure * (mappedMax / maxColor);

    color_out = vec4(color_compressed, 1.f);
}