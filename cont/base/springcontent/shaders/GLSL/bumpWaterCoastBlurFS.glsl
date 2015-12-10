#version 120

/**
 * @project Spring RTS
 * @file bumpWaterCoastBlurFS.glsl
 * @brief Input is a 0/1 bitmap and 1 indicates land.
 *        Now this shader blurs this map, so you get something like
 *        a distance to land map (->coastmap).
 * @author jK
 *
 * Copyright (C) 2008,2009.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#define res 15.0
#define renderToAtlas (gl_TexCoord[2].x > 0.5)
#define radius gl_TexCoord[2].y

uniform sampler2D tex0; //! final (fullsize) texture
uniform sampler2D tex1; //! atlas with to be updated rects

const float kernel = 1.0 / 10.0;

vec2 texelScissor = vec2(dFdx(gl_TexCoord[0].p), dFdy(gl_TexCoord[0].q)); // heightmap pos
vec2 texel0 = vec2(dFdx(gl_TexCoord[0].s), dFdy(gl_TexCoord[0].t)); // 0..1

vec4 tex2D(vec2 offset) {
	if (renderToAtlas) {
		return texture2D(tex0, gl_TexCoord[0].st + offset * texel0);
	} else {
		vec2 scissor = gl_TexCoord[0].pq + (offset * texelScissor);
		bool outOfAtlasBound = any(greaterThan(scissor,vec2(1.0))) || any(lessThan(scissor,vec2(0.0)));
		if (outOfAtlasBound) {
			return texture2D(tex1, gl_TexCoord[0].st);
		} else {
			return texture2D(tex1, gl_TexCoord[0].st + offset * texel0);
		}
	}
}

vec2 getDistRect(float d, vec2 offset) {
	vec2 dist;
	float minDist = res - d * res;
	dist.x = floor(minDist);
	float iDist = dist.x * dist.x;
	minDist *= minDist;
	dist.y = sqrt(minDist - iDist);
	dist += offset;
	return dist;
}

float sqlength(vec2 v) {
	return dot(v, v);
}


void LoopIter(inout float maxDist, inout vec3 minDist, float i) {
	// 0____1____2
	// |         |
	// |         |
	// 3    x    4
	// |         |
	// |         |
	// 5____6____7

	vec4 v1, v2;
		v1.x = tex2D(vec2(-i,  radius)).g;
		v1.y = tex2D(vec2( i,  radius)).g;
		v1.z = tex2D(vec2(-i, -radius)).g;
		v1.w = tex2D(vec2( i, -radius)).g;

		v2.x = tex2D(vec2( radius,  i)).g;
		v2.y = tex2D(vec2( radius, -i)).g;
		v2.z = tex2D(vec2(-radius,  i)).g;
		v2.w = tex2D(vec2(-radius, -i)).g;

	v1    = max(v1,    v2   );
	v1.xy = max(v1.xy, v1.zw);
	v1.x  = max(v1.x,  v1.y );
	v1.x  = max(v1.x,  maxDist );

	vec2 dist = getDistRect(v1.x, vec2(radius, i));
	//minDist.z = min(minDist.z, sqlength(dist));

	if (sqlength(dist) < minDist.z) {
		maxDist = v1.x;
		minDist = vec3(dist, sqlength(dist));
	}
}


void main() {
	if (radius < 0.5) {
		//! initialize
		gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);
		return;
	}

	if (radius > 9.5) {
		//! blur the texture in the final stage
		vec2
			groundSurrounding  = tex2D(vec2( 1.0, 1.0)).rb;
			groundSurrounding += tex2D(vec2(-1.0, 1.0)).rb;
			groundSurrounding += tex2D(vec2(-1.0,-1.0)).rb;
			groundSurrounding += tex2D(vec2( 1.0,-1.0)).rb;

		gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);

		if (groundSurrounding.x + gl_FragColor.r == 5.0) {
			gl_FragColor.r = 1.0;
		} else {
			gl_FragColor.r = 0.93 - (groundSurrounding.y + gl_FragColor.b) / 5.0;
		}

		return;
	} else if (radius > 8.5) {
		//! blur the texture in the final stage
		vec2
			blur  = texture2D(tex0, gl_TexCoord[0].st + vec2( 1.0, 1.0) * texel0).rg;
			blur += texture2D(tex0, gl_TexCoord[0].st + vec2(-1.0, 1.0) * texel0).rg;
			blur += texture2D(tex0, gl_TexCoord[0].st + vec2(-1.0,-1.0) * texel0).rg;
			blur += texture2D(tex0, gl_TexCoord[0].st + vec2( 1.0,-1.0) * texel0).rg;

		gl_FragColor = texture2D(tex0, gl_TexCoord[0].st);
		gl_FragColor.r = step(5.0, blur.x + gl_FragColor.r);
		gl_FragColor.g = mix(gl_FragColor.g, blur.y * 0.25, 0.4);
		return;
	}

	float maxValue = 0.0;
	vec3 minDist = vec3(1e9);

	// driver fails at unrolling when count is not known
	// at compile-time, so we do it manually (radius is
	// in [0.5, 8.5], so we need at most 8 iterations)
	float iter = 0.0;
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= radius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }

	//! PROCESS maxValue
	//if (maxValue == 0.0)
	//	discard;

	float fDist = 1.0 - (min(res, sqrt(minDist.z)) / res);

	if (renderToAtlas) {
		gl_FragColor = texture2D(tex0, gl_TexCoord[0].st);
	} else {
		gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);
	}

	gl_FragColor.g = max(gl_FragColor.g, fDist * fDist * fDist);
}
