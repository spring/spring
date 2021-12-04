#version 410 core

layout(location = 0) in vec3 a_vertex_xyz;
layout(location = 1) in vec2 a_texcoor_st;
layout(location = 2) in vec4 a_color_rgba;

uniform mat4 u_movi_mat;
uniform mat4 u_proj_mat;

const vec4 c_bumpmap_sgen = vec4(0.02, 0.00, 0.00, 0.0);
const vec4 c_bumpmap_tgen = vec4(0.00, 0.00, 0.02, 0.0);

out vec2 v_reflmap_tc;
out vec2 v_bumpmap_tc;
out vec4 v_color_rgba;

void main() {
	vec4 pos = vec4(a_vertex_xyz, 1.0);

	v_reflmap_tc = a_texcoor_st;
	v_bumpmap_tc = vec2(dot(pos, c_bumpmap_sgen), dot(pos, c_bumpmap_tgen));
	v_color_rgba = a_color_rgba / 255.0;

	gl_Position = u_proj_mat * u_movi_mat * pos;
}

