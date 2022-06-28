#version 460

#include "common/structs/MaterialParameters.h"
#include "common/visibilityBufferPack.glsl"

//
// In
#ifdef IS_MASKED
layout( location = 0 ) in vec2 vert_texcoord;
#endif

//
// Out
layout( location = 0 ) out uint visibility_out;

//
// Descriptors
#ifdef IS_MASKED
layout (std430, set = 2, binding = 0) readonly buffer MaterialsParametersBuffer
{
    MaterialParameters materialsParameters[MATERIALS_PARAMETERS_COUNT];
};

layout (set = 2, binding = 1) uniform sampler2D textures[TEXTURES_COUNT];
#endif

//
// Push constants
layout (push_constant) uniform PushConstants
{
    layout(offset = 4)     uint primitiveInstance;
#ifdef IS_MASKED
    layout(offset = 8)     uint materialIndex;
#endif
};

//
// Main!

void main()
{
    #ifdef IS_MASKED
        uint base_color_texture_index = materialsParameters[materialIndex].baseColorTexture;
        float tex_alpha = texture(textures[base_color_texture_index], vert_texcoord).a;
        float alpha_factor = materialsParameters[materialIndex].baseColorFactors.a;

        float alpha = tex_alpha * alpha_factor;

        if( alpha < materialsParameters[materialIndex].alphaCutoff )
            discard;
    #endif

    visibility_out = PackVisibilityBuffer(primitiveInstance, gl_PrimitiveID);
}