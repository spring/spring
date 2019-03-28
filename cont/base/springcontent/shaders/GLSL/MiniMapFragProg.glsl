#version 410 core

uniform sampler2D u_shading_tex;
uniform sampler2D u_minimap_tex;
uniform sampler2D u_infomap_tex;

uniform vec2 u_texcoor_mul;

uniform float u_infotex_mul;
uniform float u_gamma_expon;

in vec2 v_tex_coords;

layout(location = 0) out vec4 f_frag_color;

void main()
{
	vec4 shading_texel = texture(u_shading_tex, v_tex_coords * u_texcoor_mul);
	vec4 minimap_texel = texture(u_minimap_tex, v_tex_coords                );
	vec4 infomap_texel = texture(u_infomap_tex, v_tex_coords * u_texcoor_mul) - vec4(0.5);

	f_frag_color = shading_texel * minimap_texel + (infomap_texel * u_infotex_mul);
	f_frag_color.rgb = pow(f_frag_color.rgb, vec3(u_gamma_expon));
}

