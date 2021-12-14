#version 450

struct MaterialParameters
{
    vec4 baseColorFactors;
    uint baseColorTexture;

    float alphaCutoff;
};

#ifdef IS_OPAQUE
layout(early_fragment_tests) in;
#endif

// IN

#ifdef VERT_TEXCOORD0
layout( location = 0 ) in vec2 vert_texcoord0;
#endif

#ifdef VERT_COLOR0
layout( location = 1 ) in vec4 vert_color0;
#endif

// OUT

layout( location = 0 ) out vec4 frag_color;

// Push Constants

layout (push_constant) uniform PushConstants
{
    layout(offset = 8)     uint materialIndex;
};

// Descriptor Sets
layout (std140, set = MATERIAL_DS_INDEX, binding = 0) readonly buffer MaterialsParametersBuffer
{
    MaterialParameters materialsParameters[MATERIALS_PARAMETERS_COUNT];
};

layout (set = MATERIAL_DS_INDEX, binding = 1) uniform sampler2D textures[TEXTURES_COUNT];

void main() 
{
    frag_color = vec4(0.f, 0.f, 0.f, 1.f);
    #ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
    {
        uint base_color_texture_index = materialsParameters[materialIndex].baseColorTexture;
        vec4 text_color = texture(textures[base_color_texture_index], vert_texcoord0).rgba * materialsParameters[materialIndex].baseColorFactors;
        
        #ifdef IS_MASKED
        if(text_color.a < materialsParameters[materialIndex].alphaCutoff )
            discard;
        #endif
        
        frag_color = text_color;
    }
    #endif
    #ifdef VERT_COLOR0
    {
        frag_color = vec4(mix(frag_color.rgb, vert_color0.rgb, vert_color0.a), 1.f);
    }
    #endif
}
