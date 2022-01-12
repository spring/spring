#version 410 core

layout(location = 0) in vec2 a_vertex_pos;
layout(location = 1) in vec2 a_tex_coords;

out vec2 v_tex_coords;

uniform mat4 u_movi_mat;
uniform mat4 u_proj_mat;

void main()
{
	gl_Position = u_proj_mat * u_movi_mat * vec4(a_vertex_pos, 0.0, 1.0);

	v_tex_coords = a_tex_coords;
}

