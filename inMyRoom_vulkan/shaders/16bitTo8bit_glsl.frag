#version 450

layout( location = 0 ) in vec2 vert_texcoord;

#ifdef IS_R
layout( location = 0 ) out float frag_color;
#endif
#ifdef IS_RG
layout( location = 0 ) out vec2 frag_color;
#endif
#ifdef IS_RGBA
layout( location = 0 ) out vec4 frag_color;
#endif

layout(set = 0, binding = 0) uniform sampler2D inTexture;

void main() {
    #ifdef IS_R
        frag_color = texture( inTexture, vert_texcoord).r;
    #endif
    #ifdef IS_RG
        frag_color = texture( inTexture, vert_texcoord).rg;
    #endif
    #ifdef IS_RGBA
        frag_color = texture( inTexture, vert_texcoord).rgba;
    #endif
    
}