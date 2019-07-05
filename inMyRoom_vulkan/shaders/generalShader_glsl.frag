#version 450

layout( location = 0 ) in vec3 vert_position;

#ifdef VERT_TEXCOORD0
layout( location = 1 ) in vec2 vert_texcoord;
#endif


layout( location = 0 ) out vec4 frag_color;

#ifdef USE_MATERIAL
layout (set = 2, binding = 0) uniform UniformBuffer0
{
	vec4 baseColorFactor;
};
#endif

#ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
layout (set = 2, binding = 1) uniform sampler2D base_color_texture;
#endif


void main() 
{
#ifdef USE_BASE_COLOR_TEXTURE_TEXCOORD0
//	const vec4 color= baseColorFactor * texture(base_color_texture, vert_texcoord).rgba;
	const vec4 color= texture(base_color_texture, vert_texcoord).rgba;
	frag_color = color;
#else
	frag_color = vec4(0.8, 0.8, 0.8, 1.0);
#endif
}
