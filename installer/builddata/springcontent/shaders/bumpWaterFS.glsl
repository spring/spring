/**
 * @project Spring RTS
 * @file bumpWaterFS.glsl
 * @brief An extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */


//////////////////////////////////////////////////
// runtime defined constants are:
// #define SurfaceColor     vec4
// #define DiffuseColor     vec3
// #define PlaneColor       vec4  (unused)
// #define AmbientFactor    float
// #define DiffuseFactor    float (note: it is the map defined value multipled with 15x!)
// #define SpecularColor    vec3
// #define SpecularPower    float
// #define SpecularFactor   float
// #define PerlinStartFreq  float
// #define PerlinFreq       float
// #define PerlinAmp        float
// #define Speed            float
// #define FresnelMin       float
// #define FresnelMax       float
// #define FresnelPower     float
// #define ScreenInverse    vec2
// #define ViewPos          vec2
// #define MapMid           vec3
// #define SunDir           vec3
// #define ReflDistortion   float
// #define BlurBase         vec2
// #define BlurExponent     float
// #define PerlinStartFreq  float
// #define PerlinLacunarity float
// #define PerlinAmp        float
// #define TexGenPlane      vec4
// #define ShadingPlane     vec4

#define CausticDepth 0.5
#define CausticRange 0.45
#define WavesLength  0.15


//////////////////////////////////////////////////
// possible flags are:
// //#define use_heightmap
// #define use_reflection
// #define use_refraction
// #define use_shorewaves
// #define use_depth
// #define blur_reflection
// #define use_texrect

#ifdef use_texrect
  #extension GL_ARB_texture_rectangle : enable
#else
  #define texture2DRect texture2D
  #define sampler2DRect sampler2D
#endif


//////////////////////////////////////////////////
// Uniforms + Varyings

  uniform sampler2D normalmap;
  uniform sampler2D heightmap;
  uniform sampler2D caustic;
  uniform sampler2D foam;
  uniform sampler2D reflection;
  uniform sampler2DRect refraction;
  uniform sampler2D coastmap;
  uniform sampler2DRect depthmap;
  uniform sampler2D waverand;
  uniform float frame;
  uniform vec3 eyePos;

  varying vec3 eyeVec;
  varying vec3 ligVec;


//////////////////////////////////////////////////
// Screen Coordinates (normalized and screen dimensions)

#ifdef use_texrect
  vec2 screencoord = (gl_FragCoord.xy - ViewPos);
  vec2 reftexcoord = (screencoord*ScreenInverse);
#else
  vec2 screenPos = gl_FragCoord.xy - ViewPos;
  vec2 screencoord = screenPos*ScreenTextureSizeInverse;
  vec2 reftexcoord = screenPos*ScreenInverse;
#endif


//////////////////////////////////////////////////
// Depth conversion
#ifdef use_depth
  float pm15 = gl_ProjectionMatrix[2][3];
  float pm11 = gl_ProjectionMatrix[2][3];
  float convertDepthToZ(float d) {
    return pm15 / (((d * 2.0) - 1.0) + pm11);
  }
#endif

//////////////////////////////////////////////////
// shorewaves functions
#ifdef use_shorewaves
const float InvWavesLength = 1.0/WavesLength;

float smoothlimit(const float x, const float step) {
  if (x>step)
    //return step * smoothstep(1.0,step,x);
    return step - (mod(x,step)*step)/(1.0-step);
  else
    return x;
}


float waveIntensity(const float x) {
  //float front = smoothstep(1.0-0.85,0.0,abs(x-0.85));
  float front = 1.0-(abs(x-0.85))/(1.0-0.85);
  if (x<0.85)
    return max(front,x*0.5);
  else
    return front;
}
#endif

//////////////////////////////////////////////////
// MAIN()

  void main(void) {
// GET WATERDEPTH
#ifdef use_heightmap
    float waterdepth = -texture2D(heightmap,gl_TexCoord[0].st).r;
    if (waterdepth<0.0) discard;
#else
    float waterdepth;

    if ( any(greaterThanEqual(gl_TexCoord[4].pq,ShadingPlane.pq)) ||
         any(lessThanEqual(gl_TexCoord[4].pq,vec2(0.0,0.0)))
       )
    {
      waterdepth = 1.0;
    }else{
      waterdepth = 1.0 - texture2D(heightmap,gl_TexCoord[4].pq).a; //heightmap in alpha channel
      if (waterdepth==0.0) discard;
    }
    //float invwaterdepth = 1.0 - waterdepth;
#endif

#ifdef use_depth
    float tz = texture2DRect(depthmap, screencoord ).r;
    float shallowScale = clamp( abs( convertDepthToZ(tz) - convertDepthToZ(gl_FragCoord.z) )/3.0, 0.0,1.0);
#else
    float shallowScale = waterdepth;
#endif

    gl_FragColor.a = 1.0;

// NORMALMAP
    vec3 octave1 = texture2D(normalmap,gl_TexCoord[1].st).rgb;
    vec3 octave2 = texture2D(normalmap,gl_TexCoord[1].pq).rgb;
    vec3 octave3 = texture2D(normalmap,gl_TexCoord[2].st).rgb;
    vec3 octave4 = texture2D(normalmap,gl_TexCoord[2].pq).rgb;

    const float a = PerlinAmp;
    octave1 = (octave1*2.0-1.0)*a;
    octave2 = (octave2*2.0-1.0)*a*a;
    octave3 = (octave3*2.0-1.0)*a*a*a;
    octave4 = (octave4*2.0-1.0)*a*a*a*a;

    vec3 normal = octave1+octave2+octave3+octave4;
    normal = normalize( normal ).xzy;

    vec3 eVec   = normalize(eyeVec);
    float eyeNormalCos = dot(-eVec, normal);
    float angle = (1.0-abs(eyeNormalCos));


// AMBIENT & DIFFUSE
    vec3 reflectDir   = reflect(normalize(-ligVec), normal);
    float specular    = angle * pow( max(dot(reflectDir,eVec), 0.0) , SpecularPower) * SpecularFactor * shallowScale;
    const vec3 SunLow = SunDir * vec3(1.0,0.1,1.0);
    float diffuse     = pow( max( dot(normal,SunLow) ,0.0 ) ,3.0)*DiffuseFactor;
    float ambient     = smoothstep(-1.3,0.0,eyeNormalCos)*AmbientFactor;
    vec3 waterSurface = SurfaceColor.rgb + DiffuseColor*diffuse + vec3(ambient);
#ifdef use_heightmap
    float maxWaterDepth= -30.0;
    float surfaceMix   = (SurfaceColor.a + diffuse)*(waterdepth/maxWaterDepth);
    float refractDistortion = 60.0*(1.0-pow(gl_FragCoord.z,80.0))*(waterdepth/maxWaterDepth);
#else
    float surfaceMix   = (SurfaceColor.a + diffuse)*shallowScale;
    float refractDistortion = 60.0*(1.0-pow(gl_FragCoord.z,80.0))*shallowScale;
#endif


// REFRACTION
#ifdef use_refraction
  #ifdef use_texrect
    vec3 refrColor = texture2DRect(refraction, screencoord + normal.xz*refractDistortion ).rgb;
  #else
    vec3 refrColor = texture2DRect(refraction, screencoord + normal.xz*refractDistortion*ScreenInverse ).rgb;
  #endif
    gl_FragColor.rgb = mix(refrColor,waterSurface, 0.1+surfaceMix);
#else
    gl_FragColor.rgb = waterSurface;
    gl_FragColor.a   = surfaceMix + specular;
#endif


// CAUSTICS
#ifdef use_heightmap
    if ((waterdepth/maxWaterDepth)<1.0) {
      vec3 caust = texture2D(caustic,gl_TexCoord[0].pq*80.0).rgb;
      gl_FragColor.rgb = mix(gl_FragColor.rgb,refrColor+(caust*(waterdepth/maxWaterDepth)*0.25),1.0-(waterdepth/maxWaterDepth));
    }
#else
    if (waterdepth>0.0) {
      vec3 caust = texture2D(caustic,gl_TexCoord[0].pq*75.0).rgb;
  #ifdef use_refraction
      float caustBlend = smoothstep(CausticRange,0.0,abs(waterdepth-CausticDepth));
      gl_FragColor.rgb += caust*caustBlend*0.08;  
  #else
      gl_FragColor.a *= min(waterdepth*4.0,1.0);
      gl_FragColor.rgb += caust*(1.0-waterdepth)*0.6;
  #endif
    }
#endif


// SHORE WAVES
#ifdef use_shorewaves
    vec3 shorewavesColor = vec3(0.0);
    float inwaterdepth = 1.0-waterdepth;
    if (waterdepth<1.0) {
      float coastdist = texture2D(coastmap, gl_TexCoord[0].pq).r + octave3.x*0.1;
      if (coastdist>0.0) {

        vec3 wavefoam  = texture2D(foam, gl_TexCoord[3].st ).rgb;
        wavefoam += texture2D(foam, gl_TexCoord[3].pq ).rgb;
        wavefoam *= 0.5;

        vec2 wrcoord = gl_TexCoord[4].st;

        float fframe = fract(frame);
        for (float i=0.0; i<1.0; i+=0.25) {
          float wave  = i + fframe * 50.0;
          float wavef = fract(wave);
                wave -= wavef;
          float frac  = wavef * 1.4 - 0.2;
          float f = frac - coastdist;
          if (abs(f)>WavesLength) continue;
          float rand = texture2D(waverand, wrcoord + wave * 0.37 + i ).r;
          float f2   = waveIntensity( min(1.0, (WavesLength - f) * InvWavesLength));
          shorewavesColor += wavefoam * f2 * rand;
        }

        shorewavesColor *= coastdist;
      }
    }
    float iwd = smoothlimit(inwaterdepth, 0.8);
    gl_FragColor.rgb += shorewavesColor * iwd * 1.5;
#endif


// REFLECTION
#ifdef use_reflection
    //we have to mirror the Y-axis
    reftexcoord  = vec2(reftexcoord.x,1.0 - reftexcoord.y);
    reftexcoord += vec2(0.0,3.0*ScreenInverse.y) + normal.xz*0.09*ReflDistortion;

    vec3 reflColor = texture2D(reflection,reftexcoord).rgb;

  #ifdef blur_reflection
    const vec2  v = BlurBase;
    const float s = BlurExponent;
    reflColor += texture2D(reflection,reftexcoord.st+v).rgb;
    reflColor += texture2D(reflection,reftexcoord.st+v*s).rgb;
    reflColor += texture2D(reflection,reftexcoord.st+v*s*s).rgb;
    reflColor += texture2D(reflection,reftexcoord.st+v*s*s*s).rgb;
    reflColor += texture2D(reflection,reftexcoord.st+v*s*s*s*s).rgb;
    reflColor += texture2D(reflection,reftexcoord.st+v*s*s*s*s*s).rgb;
    reflColor += texture2D(reflection,reftexcoord.st+v*s*s*s*s*s*s).rgb;
    reflColor *= 0.125;
  #endif

    float fresnel    = FresnelMin + FresnelMax * pow(angle,FresnelPower);
    gl_FragColor.rgb = mix(gl_FragColor.rgb, reflColor, fresnel*shallowScale);
#endif


// SPECULAR
    gl_FragColor.rgb += specular*SpecularColor;

// FOG
    float fog = clamp( (gl_Fog.end - abs(gl_FogFragCoord)) * gl_Fog.scale ,0.0,1.0);
    gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fog );
  }
