#version 460

#include "common/toneMap.glsl"
#include "common/sRGBencode.glsl"

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

void main()
{
    float linearThreshold = 0.25f;
    uint frameIndex = framesCount - 1;

    vec4 color_out_linear;
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
            rgb_sum += ToneMap(color_exposure, linearThreshold);
        }
        color_out_linear = vec4(rgb_sum / float(max_samples_points), 1.f);
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
        color_out_linear = vec4(ToneMap(color_exposure, linearThreshold), 1.f);
    #endif

    // Gamma correction
    color_out = sRGBencode(color_out_linear);
}