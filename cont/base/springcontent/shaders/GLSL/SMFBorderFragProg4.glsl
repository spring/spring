#version 410 core

uniform sampler2D u_diffuse_tex;
uniform sampler2D u_detail_tex;

in vec3 v_vertex_xyz;
in vec4 v_color_rgba;
in vec2 v_diffuse_tc;
in vec2 v_detail_tc;

layout(location = 0) out vec4 f_color_rgba;

void main() {
	vec4 diffuse_color = texture(u_diffuse_tex, v_diffuse_tc) * (210.0 / 255.0);
	vec4 detail_color = texture(u_detail_tex, v_detail_tc) * 2.0 - 1.0;

	f_color_rgba = (diffuse_color + detail_color * 0.0) * (v_color_rgba * (1.0 / 255.0));
}

