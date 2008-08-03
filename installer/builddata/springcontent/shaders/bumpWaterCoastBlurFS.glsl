/**
 * @project Spring RTS
 * @file bumpWaterCoastBlurFS.glsl
 * @brief Input is a 0/1 bitmap and 1 indicates land.
 *        Now this shader blurs this map, so you get something like
 *        a distance to land map (->coastmap).
 * @author jK
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

uniform sampler2D texture;
uniform vec2 blurDir;
uniform int blurTextureID;

const float kernel = 1.0/10.0;

void main(void) {
  vec4 blur,blur2,blur3,blur4;
  float d1=0.0,d2,d3;

  vec2 texCoord = gl_TexCoord[0].st;

  const vec2 texel = vec2(dFdx(gl_TexCoord[0].s),dFdy(gl_TexCoord[0].t));

  // 0 distance
  gl_FragColor  = texture2D(texture, texCoord );

  if (gl_FragColor.r>0.0) {
    blur.x = texture2D(texture, texCoord+vec2(texel.x,0.0) ).r;
    blur.y = texture2D(texture, texCoord+vec2(0.0,texel.y) ).r;
    blur.z = texture2D(texture, texCoord+vec2(-texel.x,0.0) ).r;
    blur.w = texture2D(texture, texCoord+vec2(0.0,-texel.y) ).r;
    d1 = dot(blur,vec4(0.2))+gl_FragColor.r*0.2;
  }

  // 1 texel distance
  blur.x = texture2D(texture, texCoord-texel ).r;
  blur.y = texture2D(texture, texCoord-vec2(0.0,texel.y) ).r;
  blur.z = texture2D(texture, texCoord-vec2(-texel.x,texel.y) ).r;
  blur.w = texture2D(texture, texCoord-vec2(texel.x,0.0) ).r;

  blur2.x = texture2D(texture, texCoord+texel ).r;
  blur2.y = texture2D(texture, texCoord+vec2(0.0,texel.y) ).r;
  blur2.z = texture2D(texture, texCoord+vec2(-texel.x,texel.y) ).r;
  blur2.w = texture2D(texture, texCoord+vec2(texel.x,0.0) ).r;

  blur = max(blur,blur2);
  blur.xy = max(blur.xy,blur.zw);
  blur.x  = max(blur.x,blur.y);
  d2 = blur.x-kernel;

  // 2 texel distance
  blur.x = 0.0;// texture2D(texture, texCoord-2.0*texel ).r;
  blur.y = texture2D(texture, texCoord-vec2(texel.x,2.0*texel.y) ).r;
  blur.z = texture2D(texture, texCoord-vec2(0.0,2.0*texel.y) ).r;
  blur.w = texture2D(texture, texCoord-vec2(-texel.x,2.0*texel.y) ).r;

  blur2.x = 0.0;// texture2D(texture, texCoord+vec2(2.0*texel.x,-2.0*texel.y) ).r;
  blur2.y = texture2D(texture, texCoord+vec2(2.0*texel.x,-texel.y) ).r;
  blur2.z = texture2D(texture, texCoord+vec2(2.0*texel.x,0.0) ).r;
  blur2.w = texture2D(texture, texCoord+vec2(2.0*texel.x,texel.y) ).r;

  blur3.x = 0.0;// texture2D(texture, texCoord+2.0*texel ).r;
  blur3.y = texture2D(texture, texCoord+vec2(texel.x,2.0*texel.y) ).r;
  blur3.z = texture2D(texture, texCoord+vec2(0.0,2.0*texel.y) ).r;
  blur3.w = texture2D(texture, texCoord+vec2(-texel.x,2.0*texel.y) ).r;

  blur4.x = 0.0;// texture2D(texture, texCoord-vec2(2.0*texel.x,-2.0*texel.y) ).r;
  blur4.y = texture2D(texture, texCoord-vec2(2.0*texel.x,-texel.y) ).r;
  blur4.z = texture2D(texture, texCoord-vec2(2.0*texel.x,0.0) ).r;
  blur4.w = texture2D(texture, texCoord-vec2(2.0*texel.x,texel.y) ).r;

  blur = max(blur,blur2);
  blur = max(blur,blur3);
  blur = max(blur,blur4);
  blur.xy = max(blur.xy,blur.zw);
  blur.x  = max(blur.x,blur.y);
  d3 = blur.x-kernel-kernel;

  gl_FragColor.r = max(d1,max(d2,d3));
}
