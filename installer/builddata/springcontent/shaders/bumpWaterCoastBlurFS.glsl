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

const float kernel = 1.0/10.0;

vec2 texelScissor = vec2(dFdx(gl_TexCoord[0].p),dFdy(gl_TexCoord[0].q));
vec2 texel0 = vec2(dFdx(gl_TexCoord[0].s),dFdy(gl_TexCoord[0].t));

float tex2D(vec2 offset) {
  if (renderToAtlas) {
    return texture2D(tex0, gl_TexCoord[0].st + offset * texel0).g;
  } else {
    vec2 scissor = gl_TexCoord[0].pq + (offset * texelScissor);
    bool outOfAtlasBound = any(greaterThan(scissor,vec2(1.0))) || any(lessThan(scissor,vec2(0.0)));
    if (outOfAtlasBound) {
      return texture2D(tex1, gl_TexCoord[0].st).g;
    } else {
      return texture2D(tex1, gl_TexCoord[0].st + offset * texel0).g;
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

void main(void) {
  if (radius < 0.5) {
    //! initialize
    gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);
    return;
  }

  if (radius > 9.5) {
    //! blur the texture in the final stage
    vec2 groundSurrounding = texture2D(tex1, gl_TexCoord[0].st + vec2( 1.0, 1.0) * texel0).rb;
        groundSurrounding += texture2D(tex1, gl_TexCoord[0].st + vec2(-1.0, 1.0) * texel0).rb;
        groundSurrounding += texture2D(tex1, gl_TexCoord[0].st + vec2(-1.0,-1.0) * texel0).rb;
        groundSurrounding += texture2D(tex1, gl_TexCoord[0].st + vec2( 1.0,-1.0) * texel0).rb;

    gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);

    if (groundSurrounding.x + gl_FragColor.r == 5.0) {
      gl_FragColor.r = 1.0;
    } else {
      gl_FragColor.r = 0.95 - (groundSurrounding.y + gl_FragColor.b) / 6.0;
    }

    return;
  } else if (radius > 8.5) {
    //! blur the texture in the final stage
    vec2 blur = texture2D(tex0, gl_TexCoord[0].st + vec2( 1.0, 1.0) * texel0).rg;
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
  vec4 v,v2;

  for (float i = 1.0; i <= radius; i++) {
    v.x = tex2D(vec2(-i,radius));
    v.y = tex2D(vec2(i,radius));
    v.z = tex2D(vec2(-i,-radius));
    v.w = tex2D(vec2(i,-radius));

    v2.x = tex2D(vec2(radius,i));
    v2.y = tex2D(vec2(radius,-i));
    v2.z = tex2D(vec2(-radius,i));
    v2.w = tex2D(vec2(-radius,-i));

    v    = max(v,v2);
    v.xy = max(v.xy,v.zw);
    v.x  = max(v.x,v.y);
    if (v.x > maxValue.z) {
      dist = getDistRect(v.x,vec2(radius,i));
      if (sqlength(dist) < minDist.z) {
        maxValue.z = v.x;
        maxValue.x = radius;
        maxValue.y = i;
        minDist = vec3(dist,sqlength(dist));
      }
    }
  }

  v.x = tex2D(vec2(radius,radius));
  v.y = tex2D(vec2(-radius,radius));
  v.z = tex2D(vec2(-radius,-radius));
  v.w = tex2D(vec2(radius,-radius));

  v.xy = max(v.xy,v.zw);
  v.x  = max(v.x,v.y);
  if (v.x > maxValue.z) {
    dist = getDistRect(v.x,vec2(radius,radius));
    if (sqlength(dist) < minDist.z) {
      maxValue.z = v.x;
      maxValue.x = radius;
      maxValue.y = radius;
      minDist = vec3(dist,sqlength(dist));
    }
  }

  //! PROCESS maxValue
  if (maxValue.z == 0.0) discard;

  float fDist = (res - min(res,sqrt(minDist.z))) / res;
  if (renderToAtlas) {
    gl_FragColor = texture2D(tex0, gl_TexCoord[0].st);
  } else {
    gl_FragColor = texture2D(tex1, gl_TexCoord[0].st);
  }
  gl_FragColor.g = max(gl_FragColor.g, fDist*fDist*fDist);
}
