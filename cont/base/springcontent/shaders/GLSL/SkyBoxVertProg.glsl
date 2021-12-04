#version 410 core

uniform mat4 u_movi_mat;
uniform mat4 u_proj_mat;

layout(location = 0) in vec2 a_vertex_xy;
layout(location = 1) in vec3 a_texcoor_xyz;

out vec3 v_texcoor_xyz;

void main() {
	gl_Position = u_proj_mat * u_movi_mat * vec4(a_vertex_xy, 0.0, 1.0);
	v_texcoor_xyz = a_texcoor_xyz;
}

