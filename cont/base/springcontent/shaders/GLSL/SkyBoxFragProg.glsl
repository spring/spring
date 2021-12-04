#version 410 core

uniform samplerCube u_skycube_tex;

uniform float u_gamma_exponent;


in vec3 v_texcoor_xyz;

layout(location = 0) out vec4 f_color_rgba;

void main() {
	f_color_rgba = texture(u_skycube_tex, v_texcoor_xyz);
	f_color_rgba.rgb = pow(f_color_rgba.rgb, vec3(u_gamma_exponent));
}

