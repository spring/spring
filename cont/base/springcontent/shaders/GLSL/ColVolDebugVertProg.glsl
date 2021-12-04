#version 410 core

layout(location = 0) in vec3 a_vertex_xyz;

uniform mat4 u_mesh_mat;
uniform mat4 u_view_mat;
uniform mat4 u_proj_mat;

void main() {
	gl_Position = u_proj_mat * u_view_mat * u_mesh_mat * vec4(a_vertex_xyz, 1.0);
}

