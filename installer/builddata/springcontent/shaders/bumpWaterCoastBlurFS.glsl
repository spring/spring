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

uniform sampler2D tex0;
uniform vec2 blurDir;

const float kernel = 1.0/10.0;

void main(void) {
  vec2 texCoord = gl_TexCoord[0].st;

  const vec2 texel = vec2(dFdx(gl_TexCoord[0].s),dFdy(gl_TexCoord[0].t));

  // 0 distance
  gl_FragColor  = texture2D(tex0, texCoord );
  if (gl_FragColor.r>0.0) {
    vec4 blur;
    blur.x = texture2D(tex0, texCoord+vec2(texel.x,0.0) ).r;
    blur.y = texture2D(tex0, texCoord+vec2(0.0,texel.y) ).r;
    blur.z = texture2D(tex0, texCoord+vec2(-texel.x,0.0) ).r;
    blur.w = texture2D(tex0, texCoord+vec2(0.0,-texel.y) ).r;
    gl_FragColor.r = dot(blur,vec4(0.2))+gl_FragColor.r*0.2;
    return;
  }

  // 1 texel distance
  vec4 blur;
  blur.x = texture2D(tex0, texCoord-texel ).r;
  blur.y = texture2D(tex0, texCoord-vec2(0.0,texel.y) ).r;
  blur.z = texture2D(tex0, texCoord-vec2(-texel.x,texel.y) ).r;
  blur.w = texture2D(tex0, texCoord-vec2(texel.x,0.0) ).r;

  vec4 blur2;
  blur2.x = texture2D(tex0, texCoord+texel ).r;
  blur2.y = texture2D(tex0, texCoord+vec2(0.0,texel.y) ).r;
  blur2.z = texture2D(tex0, texCoord+vec2(-texel.x,texel.y) ).r;
  blur2.w = texture2D(tex0, texCoord+vec2(texel.x,0.0) ).r;

  blur = max(blur,blur2);
  blur.xy = max(blur.xy,blur.zw);
  blur.x  = max(blur.x,blur.y);
  if (blur.x>0.0) { gl_FragColor.r=blur.x-kernel; return;}

  // 2 texel distance
  blur.x = 0.0;// texture2D(tex0, texCoord-2.0*texel ).r;
  blur.y = texture2D(tex0, texCoord-vec2(texel.x,2.0*texel.y) ).r;
  blur.z = texture2D(tex0, texCoord-vec2(0.0,2.0*texel.y) ).r;
  blur.w = texture2D(tex0, texCoord-vec2(-texel.x,2.0*texel.y) ).r;

  blur2.x = 0.0;// texture2D(tex0, texCoord+vec2(2.0*texel.x,-2.0*texel.y) ).r;
  blur2.y = texture2D(tex0, texCoord+vec2(2.0*texel.x,-texel.y) ).r;
  blur2.z = texture2D(tex0, texCoord+vec2(2.0*texel.x,0.0) ).r;
  blur2.w = texture2D(tex0, texCoord+vec2(2.0*texel.x,texel.y) ).r;

  vec4 blur3;
  blur3.x = 0.0;// texture2D(tex0, texCoord+2.0*texel ).r;
  blur3.y = texture2D(tex0, texCoord+vec2(texel.x,2.0*texel.y) ).r;
  blur3.z = texture2D(tex0, texCoord+vec2(0.0,2.0*texel.y) ).r;
  blur3.w = texture2D(tex0, texCoord+vec2(-texel.x,2.0*texel.y) ).r;

  vec4 blur4;
  blur4.x = 0.0;// texture2D(tex0, texCoord-vec2(2.0*texel.x,-2.0*texel.y) ).r;
  blur4.y = texture2D(tex0, texCoord-vec2(2.0*texel.x,-texel.y) ).r;
  blur4.z = texture2D(tex0, texCoord-vec2(2.0*texel.x,0.0) ).r;
  blur4.w = texture2D(tex0, texCoord-vec2(2.0*texel.x,texel.y) ).r;

  blur = max(blur,blur2);
  blur = max(blur,blur3);
  blur = max(blur,blur4);
  blur.xy = max(blur.xy,blur.zw);
  blur.x  = max(blur.x,blur.y);
  if (blur.x>0.0) { gl_FragColor.r=blur.x-kernel-kernel; return;}
}
