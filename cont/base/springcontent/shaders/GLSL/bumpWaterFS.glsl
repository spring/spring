/**
 * @project Spring RTS
 * @file bumpWaterFS.glsl
 * @brief An extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008-2012.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

#define CausticDepth 0.5
#define CausticRange 0.45
#define WavesLength  0.15


#ifdef opt_texrect
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
  varying vec3 worldPos;
  varying vec2 clampedPotCoords;

//////////////////////////////////////////////////
// Screen Coordinates (normalized and screen dimensions)

#ifdef opt_texrect
  vec2 screencoord = (gl_FragCoord.xy - ViewPos);
  vec2 reftexcoord = (screencoord*ScreenInverse);
#else
  vec2 screenPos = gl_FragCoord.xy - ViewPos;
  vec2 screencoord = screenPos*ScreenTextureSizeInverse;
  vec2 reftexcoord = screenPos*ScreenInverse;
#endif


//////////////////////////////////////////////////
// Depth conversion
#ifdef opt_depth
  float pm15 = gl_ProjectionMatrix[2][3];
  float pm11 = gl_ProjectionMatrix[2][3];
  float convertDepthToZ(float d) {
    return pm15 / (((d * 2.0) - 1.0) + pm11);
  }
#endif


//////////////////////////////////////////////////
// shorewaves functions
#ifdef opt_shorewaves
const float InvWavesLength = 1.0/WavesLength;

float smoothlimit(const float x, const float step) {
	if (x > step)
		//return step * smoothstep(1.0,step,x);
		return step - (mod(x,step)*step)/(1.0-step);
	else
		return x;
}

vec4 waveIntensity(const vec4 v) {
	vec4 front = vec4(1.0)-(abs(v - vec4(0.85)))/vec4(1.0-0.85);
	bvec4 bs = lessThan(v, vec4(0.85));
	front = max(front, vec4(bs) * v * 0.5);
	return front;
}

#endif


//////////////////////////////////////////////////
// Shadow

#ifdef opt_shadows
  uniform mat4 shadowMatrix;
  //uniform float shadowDensity;
  uniform sampler2DShadow shadowmap;
#endif

float GetShadowOcclusion(vec3 worldPos) {
	float shadowInt = 1.0;
#ifdef opt_shadows
	vec4 vertexShadowPos = shadowMatrix * vec4(worldPos, 1.0);
	vertexShadowPos.st += vec2(0.5, 0.5);
	shadowInt = shadow2DProj(shadowmap, vertexShadowPos).r;
	shadowInt = mix(1.0, shadowInt, shadowDensity);
#endif
	return shadowInt;
}


//////////////////////////////////////////////////
// infotex

#ifdef opt_infotex
  uniform sampler2D infotex;
#endif

vec3 GetInfoTex(bool outside) {
	vec3 info = vec3(0.0);
#ifdef opt_infotex
	vec2 clampedPos = clamp(worldPos.xz, vec2(0.0), (vec2(1.0) / (ShadingPlane.xy / ShadingPlane.zw)));
	vec2 clampedPotCoords = ShadingPlane.xy * clampedPos;
	info = texture2D(infotex, clampedPotCoords).rgb * 0.5 + 0.5;
	info = info * 1.0 - 0.5;
	info = mix(info, vec3(0.0), float(outside));
#endif
	return info;
}


//////////////////////////////////////////////////
// Helpers

void GetWaterHeight(out float waterdepth, out float invwaterdepth, out bool outside, in out vec3 coastdist)
{
	outside = any(greaterThanEqual(gl_TexCoord[5].st, ShadingPlane.pq)) || any(lessThanEqual(gl_TexCoord[5].st, vec2(0.0)));
#ifdef opt_endlessocean
	if (outside) {
		outside = true;
		waterdepth = 1.0;
		invwaterdepth = 0.0;
	} else
#endif
	{
#ifdef opt_shorewaves
		coastdist = texture2D(coastmap, gl_TexCoord[0].st).rgb;
		//if (coastdist.r == 1.0) discard; //FIXME
		invwaterdepth = coastdist.b;
		waterdepth = 1.0 - invwaterdepth;
#else
		invwaterdepth = texture2D(heightmap, gl_TexCoord[5].st).a; // heightmap in alpha channel
		waterdepth = 1.0 - invwaterdepth;
		//if (waterdepth == 0.0) discard; //FIXME
#endif
	}
}


vec3 GetNormal(out vec3 octave)
{
	vec3 octave1 = texture2D(normalmap, gl_TexCoord[1].st).rgb;
	vec3 octave2 = texture2D(normalmap, gl_TexCoord[1].pq).rgb;
	vec3 octave3 = texture2D(normalmap, gl_TexCoord[2].st).rgb;
	vec3 octave4 = texture2D(normalmap, gl_TexCoord[2].pq).rgb;

	float a = PerlinAmp;
	octave1 = (octave1 * 2.0 - 1.0) * a;
	octave2 = (octave2 * 2.0 - 1.0) * a * a;
	octave3 = (octave3 * 2.0 - 1.0) * a * a * a;
	octave4 = (octave4 * 2.0 - 1.0) * a * a * a * a;

	vec3 normal = octave1 + octave2 + octave3 + octave4;
	normal = normalize(normal).xzy;

	octave = octave3;
	return normal;
}


float GetWaterDepthFromDepthBuffer(float waterdepth)
{
#ifdef opt_depth
	float tz = texture2DRect(depthmap, screencoord).r;
	float shallowScale = abs(convertDepthToZ(tz) - convertDepthToZ(gl_FragCoord.z)) / 3.0;
	shallowScale = clamp(shallowScale, 0.0, 1.0);
	return shallowScale;
#else
	return waterdepth;
#endif
}


float GetShorewaves(vec3 coast, vec3 octave, float waterdepth , float invwaterdepth)
{
	vec3 color = vec3(0.0, 0.0, 0.0);
#ifdef opt_shorewaves
	float coastdist = coast.g + octave.x * 0.1;
	if (coastdist > 0.0) {
		vec3 wavefoam = texture2D(foam, gl_TexCoord[3].st).rgb;
		wavefoam += texture2D(foam, gl_TexCoord[3].pq).rgb;
		wavefoam *= 0.5;

		if (waterdepth < 1.0) {
			vec4 waverands = texture2D(waverand, gl_TexCoord[4].pq);

			vec4 fi = vec4(0.25, 0.50, 0.75, 1.00);
			vec4 f = fract(fi + frame * 50.0);
			f = f * 1.4 - vec4(coastdist);
			f = vec4(1.0) - f * InvWavesLength;
			f = clamp(f, 0.0, 1.0);
			f = waveIntensity(f);
			vec3 shorewavesColor = wavefoam * dot(f, waverands) * coastdist;

			float iwd = smoothlimit(invwaterdepth, 0.8);
			color = shorewavesColor * iwd * 1.5;
		}

		// cliff foam
		color += 5.5 * (wavefoam * wavefoam) * (coast.r * coast.r * coast.r) * (coastdist * coastdist * coastdist * coastdist);
	}
#endif
	return color;
}


vec3 GetReflection(float angle, vec3 normal, in out float fresnel)
{
 	vec3 reflColor = vec3(0.0, 0.0, 0.0);
#ifdef opt_reflection
	// we have to mirror the Y-axis
	reftexcoord  = vec2(reftexcoord.x, 1.0 - reftexcoord.y);
	reftexcoord += vec2(0.0, 3.0 * ScreenInverse.y) + normal.xz * 0.09 * ReflDistortion;

	reflColor = texture2D(reflection, reftexcoord).rgb;

  #ifdef opt_blurreflection
	vec2  v = BlurBase;
	float s = BlurExponent;
	reflColor += texture2D(reflection, reftexcoord.st + v).rgb;
	reflColor += texture2D(reflection, reftexcoord.st + v *s).rgb;
	reflColor += texture2D(reflection, reftexcoord.st + v *s*s).rgb;
	reflColor += texture2D(reflection, reftexcoord.st + v *s*s*s).rgb;
	reflColor += texture2D(reflection, reftexcoord.st + v *s*s*s*s).rgb;
	reflColor += texture2D(reflection, reftexcoord.st + v *s*s*s*s*s).rgb;
	reflColor += texture2D(reflection, reftexcoord.st + v *s*s*s*s*s*s).rgb;
	reflColor *= 0.125;
  #endif

	fresnel = FresnelMin + FresnelMax * pow(angle, FresnelPower);
#endif
	return reflColor;
}

//////////////////////////////////////////////////
// MAIN()

void main()
{
   gl_FragColor.a = 1.0; //note: only rendered with blending iff !opt_refraction

#ifdef dbg_coastmap
    gl_FragColor = vec4(float(texture2D(coastmap, gl_TexCoord[0].st).r == 1.0));
    return;
#endif

    // GET WATERDEPTH
    bool outside = false;
    vec3 coast = vec3(0.0, 0.0, 1.0);
    float waterdepth, invwaterdepth;
    GetWaterHeight(waterdepth, invwaterdepth, outside, coast);
    float shallowScale = GetWaterDepthFromDepthBuffer(waterdepth);

  // NORMALMAP
    vec3 octave;
    vec3 normal = GetNormal(octave);

    vec3  eVec = normalize(eyeVec);
    float eyeNormalCos = dot(-eVec, normal);
    float angle = (1.0 - abs(eyeNormalCos));

  // SHADOW
    float shadowOcc = GetShadowOcclusion(worldPos + vec3(normal.x, 0.0, normal.z) * 10.0);

  // AMBIENT & DIFFUSE
    vec3 reflectDir   = reflect(normalize(-ligVec), normal);
    float specular    = angle * pow( max(dot(reflectDir, eVec), 0.0) , SpecularPower) * SpecularFactor * shallowScale;
    vec3 SunLow       = SunDir * vec3(1.0, 0.1 ,1.0);
    float diffuse     = pow( max( dot(normal, SunLow) ,0.0 ) ,3.0) * DiffuseFactor;
    float ambient     = smoothstep(-1.3, 0.0, eyeNormalCos) * AmbientFactor;
    vec3 waterSurface = SurfaceColor.rgb + DiffuseColor * diffuse + vec3(ambient);
    float surfaceMix  = (SurfaceColor.a + diffuse) * shallowScale;
    float refractDistortion = 60.0 * (1.0 - pow(gl_FragCoord.z, 80.0)) * shallowScale;


  // REFRACTION
#ifdef opt_refraction
  #ifdef opt_texrect
    vec3 refrColor = texture2DRect(refraction, screencoord + normal.xz * refractDistortion ).rgb;
  #else
    vec3 refrColor = texture2DRect(refraction, screencoord + normal.xz * refractDistortion * ScreenInverse ).rgb;
  #endif
    gl_FragColor.rgb = mix(refrColor, waterSurface, 0.1 + surfaceMix);
#else
    gl_FragColor.rgb = waterSurface;
    gl_FragColor.a   = surfaceMix + specular;
#endif


  // CAUSTICS
    if (waterdepth>0.0) {
      vec3 caust = texture2D(caustic, gl_TexCoord[0].pq * 75.0).rgb;
  #ifdef opt_refraction
      float caustBlend = smoothstep(CausticRange, 0.0, abs(waterdepth - CausticDepth));
      gl_FragColor.rgb += caust * caustBlend * 0.08;
  #else
      gl_FragColor.a *= min(waterdepth * 4.0, 1.0);
      gl_FragColor.rgb += caust * (1.0 - waterdepth) * 0.6;
  #endif
    }


  // SHORE WAVES
    gl_FragColor.rgb += shadowOcc * GetShorewaves(coast, octave, waterdepth, invwaterdepth);

  // REFLECTION
    // Schlick's approx. for Fresnel term
    float fresnel = 0.0;
    vec3 reflColor = GetReflection(angle, normal, fresnel);
    gl_FragColor.rgb = mix(gl_FragColor.rgb, reflColor, fresnel * shallowScale);

  // SPECULAR
    gl_FragColor.rgb += shadowOcc * specular * SpecularColor;

  // INFOTEX
    gl_FragColor.rgb += mix(vec3(0.0), GetInfoTex(outside), waterdepth);

  // FOG
    float fog = clamp( (gl_Fog.end - abs(gl_FogFragCoord)) * gl_Fog.scale ,0.0,1.0);
    gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fog );
}
