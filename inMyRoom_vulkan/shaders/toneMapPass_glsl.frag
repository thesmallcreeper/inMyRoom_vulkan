#version 460

//
// Out
layout( location = 0 ) out vec4 color_out;


/// 0, 0
#ifdef MULTISAMPLED_INPUT
layout (input_attachment_index = 0, set = INPUT_ATTACHMENT_SET, binding = INPUT_ATTACHMENT_BIND) uniform subpassInputMS colorInput;
#else
layout (input_attachment_index = 0, set = INPUT_ATTACHMENT_SET, binding = INPUT_ATTACHMENT_BIND) uniform subpassInput colorInput;
#endif

//
// Push constants
layout (push_constant) uniform PushConstants
{
    layout(offset = 0)     uint framesCount;
    layout(offset = 4)     float exposureFactor;
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
    float linearThreshold = 0.25f;
    uint frameIndex = framesCount - 1;

    #ifdef MULTISAMPLED_INPUT
        vec3 rgb_sum = vec3(0.f);
        uint max_samples_points = min(MULTISAMPLED_INPUT, framesCount);
        for (int i = 0; i != max_samples_points; ++i) {
            vec4 in_color = subpassLoad(colorInput, i);

            uint complete_cycles = frameIndex / MULTISAMPLED_INPUT;
            uint cycle_modulo = frameIndex % MULTISAMPLED_INPUT;

            uint evals_count = complete_cycles + ((i <= cycle_modulo)? 1 : 0);
            float this_frameCountFactor = 1.f / float(evals_count);
            #ifdef CHECK_ALPHA
                float alpha = in_color.a;
                if (alpha == 1.f) {
                    this_frameCountFactor = 1.f;
                }
            #endif

            vec3 color_exposure = vec3(in_color) * this_frameCountFactor * exposureFactor;

            float maxColor = max(color_exposure.x, max(color_exposure.y, color_exposure.z));
            float mappedMax = RangeCompress(maxColor, linearThreshold);
            vec3 color_compressed = color_exposure * (mappedMax / maxColor);

            rgb_sum += color_compressed, vec3(1.f/2.2f);
        }
        color_out = vec4(rgb_sum / float(max_samples_points), 1.f);
    #else
        float frameCountFactor = 1.f / float(framesCount);
        vec4 in_color = subpassLoad(colorInput);

        float this_frameCountFactor = frameCountFactor;
        #ifdef CHECK_ALPHA
            float alpha = in_color.a;
            if (alpha == 1.f) {
                this_frameCountFactor = 1.f;
            }
        #endif

        vec3 color_exposure = vec3(in_color) * this_frameCountFactor * exposureFactor;

        float maxColor = max(color_exposure.x, max(color_exposure.y, color_exposure.z));
        float mappedMax = RangeCompress(maxColor, linearThreshold);
        vec3 color_compressed = color_exposure * (mappedMax / maxColor);

        color_out = vec4(color_compressed, 1.f);
    #endif
}