#version 450

layout( location = 0 ) in vec3 vert_position;

#ifdef VERT_TEXCOORD0
layout( location = 1 ) in vec2 vert_texcoord;
#endif


layout( location = 0 ) out vec4 frag_color;


#ifdef FRAG_BASE_COLOR_TEXTURE_TEXCOORD0
layout (set = 2, binding = 0) uniform sampler2D base_color_texture;
#endif


void main() 
{
#ifdef FRAG_BASE_COLOR_TEXTURE_TEXCOORD0
	frag_color = texture(base_color_texture, vert_texcoord).rgba;
#else
	frag_color = vec4(1.0, 1.0, 1.0, 1.0);
#endif
}
