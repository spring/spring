#version 410 core

uniform sampler2D reflmap_tex; // TU0
uniform sampler2D bumpmap_tex; // TU1

uniform vec2 u_forward_vec; // x,z,0,0
uniform vec3 u_gamma_expon;

in vec4 v_color_rgba;
in vec2 v_reflmap_tc;
in vec2 v_bumpmap_tc;

const vec2 bumpmap_ofs = vec2(-0.50, -0.50);
const vec2 bumpmap_mul = vec2( 0.02,  0.02);

layout(location = 0) out vec4 f_frag_color;

void main() {
	vec4 bumpmap_texel = texture(bumpmap_tex, v_bumpmap_tc);
	vec4 reflmap_texel;

	vec2 bumpmap_dp;
	vec2 reflmap_tc;

	bumpmap_dp.x = dot(bumpmap_texel.xy + bumpmap_ofs, vec2( u_forward_vec.y, u_forward_vec.x));
	bumpmap_dp.y = dot(bumpmap_texel.xy + bumpmap_ofs, vec2(-u_forward_vec.x, u_forward_vec.y));
	reflmap_tc = v_reflmap_tc + bumpmap_dp * bumpmap_mul;

	// dependent lookup
	reflmap_texel = texture(reflmap_tex, reflmap_tc);
	f_frag_color = reflmap_texel * (v_color_rgba + vec4(0.0, 0.0, 0.0, bumpmap_dp.y * 0.03));

	f_frag_color.rgb = pow(f_frag_color.rgb, u_gamma_expon);
}

