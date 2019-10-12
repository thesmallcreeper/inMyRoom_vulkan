#version 450

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

// Descriptor Sets

#ifdef USE_MATERIAL
layout (set = 1, binding = 0) uniform UniformBuffer0
{
    vec4 baseColorFactor;
};
#endif

#ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
layout (set = 1, binding = 1) uniform sampler2D base_color_texture;
#endif


void main() 
{
    frag_color = vec4(1.0, 1.0, 1.0, 1.0);
    #ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
    {
        vec4 text_color= texture(base_color_texture, vert_texcoord0).rgba;
        frag_color = text_color;
    }
    #endif
    #ifdef VERT_COLOR0
    {
        frag_color = vec4(mix(frag_color.rgb, vert_color0.rgb, vert_color0.a), 1.0);
    }
    #endif
}
