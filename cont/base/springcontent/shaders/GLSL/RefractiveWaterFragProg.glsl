#version 410 core

uniform sampler2D     reflmap_tex; // TU0
uniform sampler2D     bumpmap_tex; // TU1
uniform sampler2DRect subsurf_tex; // TU2
uniform sampler2D     shading_tex; // TU3

uniform vec2 u_forward_vec; // x,z,0,0
uniform vec3 u_gamma_expon;
uniform vec2 u_texrect_size;

in vec4 v_color_rgba;
in vec2 v_reflmap_tc;
in vec2 v_bumpmap_tc;
in vec2 v_subsurf_tc; // unused
in vec2 v_shading_tc;

const vec2 bumpmap_ofs = vec2(-0.50, -0.50);
const vec2 bumpmap_mul = vec2( 0.02,  0.02);

layout(location = 0) out vec4 f_frag_color;

void main() {
	vec4 bumpmap_texel = texture(bumpmap_tex, v_bumpmap_tc);
	vec4 shading_texel = texture(shading_tex, v_shading_tc);

	vec2 bumpmap_dp;
	vec2 reflmap_tc;
	vec2 subsurf_tc;

	bumpmap_dp.x = dot(bumpmap_texel.xy + bumpmap_ofs, vec2( u_forward_vec.y, u_forward_vec.x));
	bumpmap_dp.y = dot(bumpmap_texel.xy + bumpmap_ofs, vec2(-u_forward_vec.x, u_forward_vec.y));

	reflmap_tc = v_reflmap_tc + bumpmap_dp * bumpmap_mul;
	// water depth (stored as 255 + groundheight) is encoded in alpha-channel
	subsurf_tc = (vec2(1.0) - shading_texel.a) * u_texrect_size * bumpmap_dp;
	subsurf_tc = gl_FragCoord.xy + subsurf_tc * gl_FragCoord.w;

	// dependent lookups
	vec4 reflmap_texel = texture(reflmap_tex, reflmap_tc);
	vec4 subsurf_texel = texture(subsurf_tex, subsurf_tc);
	vec4 blend_factors = vec4(0.5, 0.5, 0.5, 0.0) * shading_texel.a + vec4(0.4, 0.4, 0.4, 0.0);

	f_frag_color = reflmap_texel * v_color_rgba;
	f_frag_color = mix(f_frag_color, subsurf_texel, blend_factors);

	f_frag_color.rgb = pow(f_frag_color.rgb, u_gamma_expon);
}

