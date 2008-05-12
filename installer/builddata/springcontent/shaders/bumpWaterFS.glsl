/**
 * @project Spring RTS
 * @file bumpWaterVS.glsl
 * @brief An extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

//////////////////////////////////////////////////
// runtime defined constants are:
// #define SurfaceColor   vec4
// #define SpecularColor  vec3
// #define SpecularFactor float
// #define MapMid         vec3
// #define SunDir         vec3
// #define FresnelMin     float
// #define FresnelMax     float
// #define FresnelPower   float
// #define ScreenInverse  vec2
// #define ViewPos        vec2
// #define SunDir         vec3

#extension GL_ARB_texture_rectangle : enable
//#extension GL_ARB_shader_texture_lod : enable

  uniform sampler2D normalmap;
  uniform sampler2D heightmap;
  uniform sampler2D caustic;
  uniform sampler2D foam;
  uniform sampler2D reflection;
  uniform sampler2DRect refraction;
  uniform float frame;
  uniform vec3 eyePos;

  varying vec3 eyeVec;
  varying vec3 ligVec;

  void main(void) {
#ifdef use_heightmap
    float waterdepth = texture2D(heightmap,gl_TexCoord[0].st).r;
    if (waterdepth<0.0) {
#else
    float waterdepth = texture2D(heightmap,gl_TexCoord[0].pq).a; //heightmap in alpha channel
    if (waterdepth<1.0) {
      float invwaterdepth = 1.0-waterdepth;
#endif
      gl_FragColor.a = 1.0;

      vec3 eVec = normalize(eyeVec);

      vec3 octave4 = texture2D(normalmap,(gl_TexCoord[0].st+frame*vec2(-1.0,1.0))*40.0).rgb;
      vec3 octave3 = texture2D(normalmap,(gl_TexCoord[0].st+frame)*100.0,-1.0).rgb;
      vec3 octave2 = texture2D(normalmap,(gl_TexCoord[0].st-frame)*20.0,-1.0).rgb;
      vec3 octave1 = texture2D(normalmap,(gl_TexCoord[0].st+frame)*5.0).rgb;

      vec3 normal = (octave1+octave2+octave3+octave4)*0.5;
      normal = normalize( normal - 1.0).xzy;

      float eyeNormalCos = dot(-eVec, normal);

      vec3 reflectDir = reflect(normalize(-ligVec), normal);
      float specular  = pow( max(dot(reflectDir,eVec), 0.0) , SpecularFactor);
      float diffuse   = pow( dot(normal,SunDir) ,3.0)*0.1;
      float ambient   = smoothstep(-1.3,0.0,eyeNormalCos);
      vec3  waterSurface = SurfaceColor.rgb + vec3(diffuse+ambient);
#ifdef use_heightmap
      float maxWaterDepth= -30.0;
      float surfaceMix   = (SurfaceColor.a + diffuse)*(waterdepth/maxWaterDepth);
      float refractDistortion = 60.0*(1.0-pow(gl_FragCoord.z,80.0))*(waterdepth/maxWaterDepth);
#else
      float surfaceMix   = (SurfaceColor.a + diffuse)*(1.0-waterdepth);
      float refractDistortion = 60.0*(1.0-pow(gl_FragCoord.z,80.0))*invwaterdepth;
#endif

#ifdef use_refraction
      vec3 refrColor   = texture2DRect(refraction, gl_FragCoord.xy-ViewPos + normal.xz*refractDistortion ).rgb;
      gl_FragColor.rgb = mix(waterSurface,refrColor, surfaceMix);
#else
      gl_FragColor.rgb = waterSurface;
      gl_FragColor.a   = surfaceMix + specular;
#endif

#ifdef use_heightmap
      if ((waterdepth/maxWaterDepth)<1.0) {
        vec3 caust = texture2D(caustic,gl_TexCoord[0].st*80.0).rgb;
        gl_FragColor.rgb = mix(gl_FragColor.rgb,refrColor+(caust*(waterdepth/maxWaterDepth)*0.22),1.0-(waterdepth/maxWaterDepth));
#else
      if (waterdepth>0.0) {
        vec3 caust = texture2D(caustic,gl_TexCoord[0].st*75.0).rgb;
  #ifdef use_refraction
        gl_FragColor.rgb = mix(gl_FragColor.rgb,refrColor+(caust*invwaterdepth)*0.22,waterdepth);
  #else
        gl_FragColor.a *= min(invwaterdepth*4.0,1.0);
        gl_FragColor.rgb += caust*waterdepth*0.6;
  #endif
#endif
      }

      float angle = (1.0-abs(eyeNormalCos));

#ifdef use_reflection
      float fresnel    = FresnelMin + FresnelMax * pow(angle,FresnelPower);
      vec3 reflColor   = texture2D(reflection,vec2(0.0,1.0) - (gl_FragCoord.xy-ViewPos)*ScreenInverse + normal.xz*0.09).rgb;
      gl_FragColor.rgb = mix(gl_FragColor.rgb, reflColor, fresnel);
#endif

      gl_FragColor.rgb += SpecularColor*specular*angle;
    }else{ discard; }
  }
