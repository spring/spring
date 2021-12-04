#version 410 core
#define SMF_TEXSQR_SIZE 1024.0

layout(location = 0) in vec3 a_vertex_xyz;
layout(location = 1) in vec4 a_color_rgba;

uniform mat4 u_movi_mat;
uniform mat4 u_proj_mat;
uniform ivec3 u_diffuse_tex_sqr;

const vec4 detail_plane_s = vec4(0.005, 0.000, 0.005, 0.5);
const vec4 detail_plane_t = vec4(0.000, 0.005, 0.000, 0.5);

out vec3 v_vertex_xyz;
out vec4 v_color_rgba;
out vec2 v_diffuse_tc;
out vec2 v_detail_tc;

void main() {
	vec4 vertex_xyzw = vec4(a_vertex_xyz, 1.0);

	v_vertex_xyz = a_vertex_xyz;
	v_color_rgba = a_color_rgba;

	v_diffuse_tc.s = dot(vertex_xyzw, vec4(1.0 / SMF_TEXSQR_SIZE, 0.0, 0.0                  , -u_diffuse_tex_sqr.x * 1.0));
	v_diffuse_tc.t = dot(vertex_xyzw, vec4(0.0                  , 0.0, 1.0 / SMF_TEXSQR_SIZE, -u_diffuse_tex_sqr.y * 1.0));
	v_detail_tc.s = dot(vertex_xyzw, detail_plane_s);
	v_detail_tc.t = dot(vertex_xyzw, detail_plane_t);

	gl_Position = u_proj_mat * u_movi_mat * vertex_xyzw;
}

