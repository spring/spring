#version 410 core

/**
 * @project Spring RTS
 * @file bumpWaterCoastBlurFS.glsl
 * @brief Input is a 0/1 bitmap where 1 indicates land. Shader
 *        blurs this map, resulting in a distance-to-land map
 *        a.k.a. coastmap.
 * @author jK
 *
 * Copyright (C) 2008,2009.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#define res 15.0
#define renderToAtlas (args.x == 1)
#define kernelRadius (args.y)


uniform sampler2D tex0; // final (fullsize) texture
uniform sampler2D tex1; // atlas with to be updated rects
uniform     ivec2 args; // .x := renderToAtlas, .y := kernelRadius

in vec4 vTexCoord;
layout(location = 0) out vec4 fFragColor;


vec2 texelScissor = vec2(dFdx(vTexCoord.p), dFdy(vTexCoord.q)); // heightmap pos
vec2 texel0 = vec2(dFdx(vTexCoord.s), dFdy(vTexCoord.t)); // 0..1


vec4 tex2D(vec2 offset) {
	if (renderToAtlas)
		return texture(tex0, vTexCoord.st + offset * texel0);

	vec2 scissor = vTexCoord.pq + (offset * texelScissor);

	// check if outside atlas bounds
	if (any(greaterThan(scissor, vec2(1.0))) || any(lessThan(scissor, vec2(0.0))))
		return texture(tex1, vTexCoord.st);

	return texture(tex1, vTexCoord.st + offset * texel0);
}

vec2 getDistRect(float d, vec2 offset) {
	float minDist = (res - d * res) * (res - d * res);

	vec2 dist;
	dist.x = floor(minDist);
	dist.y = sqrt(minDist - dist.x * dist.x);
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
		v1.x = tex2D(vec2(-i,  kernelRadius)).g;
		v1.y = tex2D(vec2( i,  kernelRadius)).g;
		v1.z = tex2D(vec2(-i, -kernelRadius)).g;
		v1.w = tex2D(vec2( i, -kernelRadius)).g;

		v2.x = tex2D(vec2( kernelRadius,  i)).g;
		v2.y = tex2D(vec2( kernelRadius, -i)).g;
		v2.z = tex2D(vec2(-kernelRadius,  i)).g;
		v2.w = tex2D(vec2(-kernelRadius, -i)).g;

	v1    = max(v1,    v2   );
	v1.xy = max(v1.xy, v1.zw);
	v1.x  = max(v1.x,  v1.y );
	v1.x  = max(v1.x,  maxDist );

	vec2 dist = getDistRect(v1.x, vec2(kernelRadius, i));
	//minDist.z = min(minDist.z, sqlength(dist));

	if (sqlength(dist) < minDist.z) {
		maxDist = v1.x;
		minDist = vec3(dist, sqlength(dist));
	}
}


void main() {
	if (kernelRadius <= 0) {
		// initial stage; copy atlas-texture
		fFragColor = texture(tex1, vTexCoord.st);
		return;
	}

	if (kernelRadius == 10) {
		// final stage; blur the texture
		vec2
			groundSurrounding  = tex2D(vec2( 1.0,  1.0)).rb;
			groundSurrounding += tex2D(vec2(-1.0,  1.0)).rb;
			groundSurrounding += tex2D(vec2(-1.0, -1.0)).rb;
			groundSurrounding += tex2D(vec2( 1.0, -1.0)).rb;

		fFragColor = texture(tex1, vTexCoord.st);

		if (groundSurrounding.x + fFragColor.r == 5.0) {
			fFragColor.r = 1.0;
		} else {
			fFragColor.r = 0.93 - (groundSurrounding.y + fFragColor.b) / 5.0;
		}

		return;
	}

	if (kernelRadius == 9) {
		// penultimate stage
		vec2
			blur  = texture(tex0, vTexCoord.st + vec2( 1.0,  1.0) * texel0).rg;
			blur += texture(tex0, vTexCoord.st + vec2(-1.0,  1.0) * texel0).rg;
			blur += texture(tex0, vTexCoord.st + vec2(-1.0, -1.0) * texel0).rg;
			blur += texture(tex0, vTexCoord.st + vec2( 1.0, -1.0) * texel0).rg;

		fFragColor = texture(tex0, vTexCoord.st);
		fFragColor.r = step(5.0, blur.x + fFragColor.r);
		fFragColor.g = mix(fFragColor.g, blur.y * 0.25, 0.4);
		return;
	}

	vec3 minDist = vec3(1e9);

	// driver fails at unrolling when count is not known
	// at compile-time, so we do it manually (radius is
	// in [0.5, 8.5], so we need at most 8 iterations)
	float iter = 0.0;
	float maxValue = 0.0;

	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }
	if (iter <= kernelRadius) { LoopIter(maxValue, minDist, iter); iter += 1.0; }

	// PROCESS maxValue
	//if (maxValue == 0.0)
	//	discard;

	float fDist = 1.0 - (min(res, sqrt(minDist.z)) / res);

	if (renderToAtlas) {
		fFragColor = texture(tex0, vTexCoord.st);
	} else {
		fFragColor = texture(tex1, vTexCoord.st);
	}

	fFragColor.g = max(fFragColor.g, fDist * fDist * fDist);
}
