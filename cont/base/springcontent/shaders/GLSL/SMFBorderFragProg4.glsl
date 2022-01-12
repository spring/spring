#version 410 core

uniform sampler2DArray u_diffuse_tex;
uniform sampler2D      u_detail_tex;

uniform ivec3 u_diffuse_tex_sqr;
uniform float u_gamma_exponent;

in vec3 v_vertex_xyz;
in vec4 v_color_rgba;
in vec2 v_diffuse_tc;
in vec2 v_detail_tc;

// SMF_INTENSITY_MULT
const vec4 diffuse_mult = vec4(210.0 / 255.0, 210.0 / 255.0, 210.0 / 255.0, 1.0);

layout(location = 0) out vec4 f_color_rgba;

void main() {
	vec4 diffuse_color = texture(u_diffuse_tex, vec3(v_diffuse_tc, u_diffuse_tex_sqr.z)) * diffuse_mult;
	vec4 detail_color = texture(u_detail_tex, v_detail_tc) * 2.0 - 1.0;

	f_color_rgba = (diffuse_color + detail_color) * (v_color_rgba * (1.0 / 255.0));
	f_color_rgba.rgb = pow(f_color_rgba.rgb, vec3(u_gamma_exponent));
}

