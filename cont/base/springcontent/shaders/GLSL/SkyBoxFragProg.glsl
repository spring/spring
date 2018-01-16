#version 410 core

uniform samplerCube u_skycube_tex;

in vec3 v_texcoor_xyz;

layout(location = 0) out vec4 f_color_rgba;

void main() {
	f_color_rgba = texture(u_skycube_tex, v_texcoor_xyz);
}

