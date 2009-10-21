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

#version 120
#define res 15.0
#define renderToAtlas (gl_TexCoord[2].x > 0.5)
#define radius gl_TexCoord[2].y

uniform sampler2D tex0; //! final (fullsize) texture
uniform sampler2D tex1; //! atlas with to be updated rects

const float kernel = 1.0 / 10.0;

vec2 texelScissor = vec2(dFdx(gl_TexCoord[0].p), dFdy(gl_TexCoord[0].q));
vec2 texel0 = vec2(dFdx(gl_TexCoord[0].s), dFdy(gl_TexCoord[0].t));

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
  return v.x*v.x + v.y*v.y;
}


float[6] LoopIter(float[6] mvmdIn, float i) {
	vec3 maxValue = vec3(mvmdIn[0], mvmdIn[1], mvmdIn[2]);
	vec3 minDist  = vec3(mvmdIn[3], mvmdIn[4], mvmdIn[5]);

	vec2 dist;
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

	if (v1.x > maxValue.z) {
		dist = getDistRect(v1.x, vec2(radius, i));

		if (sqlength(dist) < minDist.z) {
			maxValue.z = v1.x;
			maxValue.x = radius;
			maxValue.y = i;
			minDist = vec3(dist, sqlength(dist));
		}
	}

	float mvmdOut[6];
		mvmdOut[0] = maxValue.x;
		mvmdOut[1] = maxValue.y;
		mvmdOut[2] = maxValue.z;
		mvmdOut[3] = minDist.x;
		mvmdOut[4] = minDist.y;
		mvmdOut[5] = minDist.z;

	return mvmdOut;
}


void main(void) {
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

		if (blur.x + gl_FragColor.r == 5.0) {
			gl_FragColor.r = 1.0;
		} else {
			gl_FragColor.r = 0.0;
		}

		gl_FragColor.g = gl_FragColor.g*0.4 + (blur.y*0.25)*0.6;
		return;
	}

	//! CALC DISTFIELD
	//!
	//!  ____1____
	//! |         |
	//! |         |
	//! 3    x    4
	//! |         |
	//! |____2____|

	vec3 maxValue = vec3(0.0);
	vec3 minDist = vec3(1e9);

	vec2 dist;
	vec4 v, v2;



	float iter = 1.0;
	float args[6];
		args[0] = maxValue.x;
		args[1] = maxValue.y;
		args[2] = maxValue.z;
		args[3] = minDist.x;
		args[4] = minDist.y;
		args[5] = minDist.z;

	// driver fails at unrolling when count is not known
	// at compile-time, so we do it manually (radius is
	// in [0.5, 8.5], so we need at most 8 iterations)
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }
	if (iter <= radius) { args = LoopIter(args, iter); iter += 1.0; }

	maxValue.x = args[0]; maxValue.y = args[1]; maxValue.z = args[2];
	minDist.x  = args[3]; minDist.y  = args[4]; minDist.z  = args[5];

	/*
	for (float i = 1.0; i <= radius; i++) {
		v.x = tex2D(vec2(-i,  radius)).g;
		v.y = tex2D(vec2( i,  radius)).g;
		v.z = tex2D(vec2(-i, -radius)).g;
		v.w = tex2D(vec2( i, -radius)).g;

		v2.x = tex2D(vec2( radius,  i)).g;
		v2.y = tex2D(vec2( radius, -i)).g;
		v2.z = tex2D(vec2(-radius,  i)).g;
		v2.w = tex2D(vec2(-radius, -i)).g;

		v    = max(v,    v2  );
		v.xy = max(v.xy, v.zw);
		v.x  = max(v.x,  v.y );

		if (v.x > maxValue.z) {
			dist = getDistRect(v.x, vec2(radius, i));
			if (sqlength(dist) < minDist.z) {
				maxValue.z = v.x;
				maxValue.x = radius;
				maxValue.y = i;
				minDist = vec3(dist, sqlength(dist));
			}
		}
	}
	*/

	// final iteration
	v.x = tex2D(vec2( radius,  radius)).g;
	v.y = tex2D(vec2(-radius,  radius)).g;
	v.z = tex2D(vec2(-radius, -radius)).g;
	v.w = tex2D(vec2( radius, -radius)).g;

	v.xy = max(v.xy, v.zw);
	v.x  = max(v.x,  v.y );

	if (v.x > maxValue.z) {
		dist = getDistRect(v.x, vec2(radius, radius));
		if (sqlength(dist) < minDist.z) {
			maxValue.z = v.x;
			maxValue.x = radius;
			maxValue.y = radius;
			minDist = vec3(dist, sqlength(dist));
		}
	}

	//! PROCESS maxValue
	if (maxValue.z == 0.0)
		discard;

	float fDist = (res - min(res, sqrt(minDist.z))) / res;

	if (renderToAtlas) {
		gl_FragColor = texture2D(tex0, gl_TexCoord[0].st);
	} else {
		gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);
	}

	gl_FragColor.g = max(gl_FragColor.g, fDist * fDist * fDist);
}
