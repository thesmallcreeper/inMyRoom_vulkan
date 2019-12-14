#version 450

struct MaterialMapsIndexes
{
    uint baseColor;
    uint metallic;
    uint roughness;
    uint normal;
    uint occlusion;
    uint emissive;
};

struct MaterialParameters
{
    vec4 baseColorFactors;
    vec4 _placeholder;
};

#ifdef USE_EARLY_FRAGMENT_TESTS
layout(early_fragment_tests) in;
#endif

// IN

layout( location = 0 ) in vec3 vert_position;


#ifdef VERT_TEXCOORD0
layout( location = 1 ) in vec2 vert_texcoord0;
#endif

#ifdef VERT_TEXCOORD1
layout( location = 2 ) in vec2 vert_texcoord1;
#endif

#ifdef VERT_COLOR0
layout( location = 3 ) in vec4 vert_color0;
#endif

// OUT

layout( location = 0 ) out vec4 frag_color;

// Push Constants

#ifdef USE_MATERIAL

layout (push_constant) uniform PushConstants
{
    layout(offset = 96)     uint materialIndex;
    layout(offset = 96 + 4) MaterialMapsIndexes mapsIndexes;
};

// Descriptor Sets
layout (set = 1, binding = 0) uniform MaterialsParametersBuffer
{
    MaterialParameters materialsParameters[MATERIALS_PARAMETERS_COUNT];
};

#endif

#ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
layout (set = 1, binding = 1) uniform sampler2D textures[TEXTURES_COUNT];
#endif



void main() 
{
    frag_color = vec4(1.0, 1.0, 1.0, 1.0);
    #ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
    {
        uint base_color_texture_index = mapsIndexes.baseColor;
        vec4 text_color= texture(textures[base_color_texture_index], vert_texcoord0).rgba;
        if(text_color.a < 0.3)
            discard;
        
        frag_color = text_color;
    }
    #endif
    #ifdef VERT_COLOR0
    {
        frag_color = vec4(mix(frag_color.rgb, vert_color0.rgb, vert_color0.a), 1.0);
    }
    #endif
}
