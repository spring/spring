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
// ancient
#extension GL_ARB_texture_rectangle : enable
#else
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

#ifdef opt_infotex
uniform sampler2D infotex;
#endif

#ifdef opt_shadows
uniform sampler2DShadow shadowmap;

uniform mat4 shadowMatrix;
// added via SetupUniforms or as a constant definition
// uniform float shadowDensity;
#endif

uniform mat4 projMatrix;



uniform float frame;
// neither added via SetupUniforms nor as a constant definition
uniform float gammaExponent;

uniform vec3 eyePos;
uniform vec3 fogColor;
uniform vec3 fogParams;

in vec3 eyeVec;
in vec3 sunVec;
in vec3 worldPos;
in vec2 clampedPotCoords;

in vec4 texCoord0;
in vec4 texCoord1;
in vec4 texCoord2;
in vec4 texCoord3;
in vec4 texCoord4;
in vec4 texCoord5;

in float fogCoord;

layout(location = 0) out vec4 fragColor;


vec2 clampedWorldPos = clamp(worldPos.xz, vec2(0.0), (vec2(1.0) / TexGenPlane.xy));



//////////////////////////////////////////////////
// Screen Coordinates (normalized and screen dimensions)

#ifdef opt_texrect
vec2 screencoord = gl_FragCoord.xy - ViewPos;
vec2 reftexcoord = screencoord * ScreenInverse;
#else
vec2 screencoord = (gl_FragCoord.xy - ViewPos) * ScreenTextureSizeInverse;
vec2 reftexcoord = (gl_FragCoord.xy - ViewPos) * ScreenInverse;
#endif



//////////////////////////////////////////////////
// Depth conversion
#ifdef opt_depth
float ConvertDepthToEyeZ(float d) {
	float pm14 = projMatrix[3].z;
	float pm10 = projMatrix[2].z;

	return (pm14 / (d * -2.0 + 1.0 - pm10));
}
#endif



//////////////////////////////////////////////////
// shorewaves functions
#ifdef opt_shorewaves
const float InvWavesLength = 1.0 / WavesLength;

float smoothlimit(const float x, const float edge) {
	float limitcurv = edge - (mod(x,edge) * edge) / (1.0 - edge);
	return mix(x, limitcurv, step(edge, x));
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

float GetShadowOcclusion(vec3 worldPos) {
	#ifdef opt_shadows
	vec4 vertexShadowPos = shadowMatrix * vec4(worldPos, 1.0);
	return mix(1.0, textureProj(shadowmap, vertexShadowPos), shadowDensity);
	#endif
	return 1.0;
}



//////////////////////////////////////////////////
// infotex

vec3 GetInfoTex(float outside) {
	vec3 info = vec3(0.0);
	#ifdef opt_infotex
	vec2 clampedPotCoords = ShadingPlane.xy * clampedWorldPos;
	info = (texture(infotex, clampedPotCoords).rgb - 0.5) * 0.5;
	info = mix(info, vec3(0.0), outside);
	#endif
	return info;
}






//////////////////////////////////////////////////
// Helpers

vec3 GetWaterHeight(inout vec2 coastdist)
{
	vec3 depths = vec3(0.0, 0.0, 0.0);

	#ifdef opt_shorewaves
	vec3 coast = texture(coastmap, texCoord0.st).rgb;

	coastdist = coast.rg;
	depths.y = coast.b;
	#else
	depths.y = texture(heightmap, texCoord5.st).a; // heightmap in alpha channel
	#endif

	#ifdef opt_endlessocean
	depths.z = min(1.0, distance(clampedWorldPos, worldPos.xz));
	depths.y = mix(depths.y, 1.0, step(0.5, depths.z));
	#endif

	depths.x = 1.0 - depths.y;

	return depths;
}


vec3 GetNormal(out vec3 octave)
{
	vec3 octave1 = texture(normalmap, texCoord1.st).rgb;
	vec3 octave2 = texture(normalmap, texCoord1.pq).rgb;
	vec3 octave3 = texture(normalmap, texCoord2.st).rgb;
	vec3 octave4 = texture(normalmap, texCoord2.pq).rgb;

	float a = PerlinAmp;
	octave1 = (octave1 * 2.0 - 1.0) * a;
	octave2 = (octave2 * 2.0 - 1.0) * a * a;
	octave3 = (octave3 * 2.0 - 1.0) * a * a * a;
	octave4 = (octave4 * 2.0 - 1.0) * a * a * a * a;

	vec3 normal = normalize(octave1 + octave2 + octave3 + octave4);

	octave = octave3;

	return normal.xzy;
}


float GetWaterDepthFromDepthBuffer(float waterdepth)
{
	#ifdef opt_depth
	// calculate difference between texel-z and fragment-z; convert
	// since both are non-linear mappings from 0=dr.min to 1=dr.max
	// absolute differences larger than 3 elmos are clamped to 1
	float  texZ = ConvertDepthToEyeZ(texture2DRect(depthmap, screencoord).r);
	float fragZ = ConvertDepthToEyeZ(gl_FragCoord.z);
	float diffZ = abs(texZ - fragZ) * 0.333;

	return clamp(diffZ, 0.0, 1.0);
	#else
	return waterdepth;
	#endif
}


vec3 GetShorewaves(vec2 coast, vec3 octave, float waterdepth , float invwaterdepth)
{
	vec3 color = vec3(0.0, 0.0, 0.0);

	#ifdef opt_shorewaves
	float coastdist = coast.g + octave.x * 0.1;

	if (coastdist > 0.0) {
		// no shorewaves/foam under terrain (is 0.0 underground, 1.0 else)
		float underground = 1.0 - step(1.0, invwaterdepth);

		vec3 wavefoam = texture(foam, texCoord3.st).rgb;
		wavefoam += texture(foam, texCoord3.pq).rgb;
		wavefoam *= 0.5;

		// shorewaves
		vec4 waverands = texture(waverand, texCoord4.pq);

		vec4 fi = vec4(0.25, 0.50, 0.75, 1.00);
		vec4 f = fract(fi + frame * 50.0);
		f = f * 1.4 - vec4(coastdist);
		f = vec4(1.0) - f * InvWavesLength;
		f = clamp(f, 0.0, 1.0);
		f = waveIntensity(f);
		float intensity = dot(f, waverands) * coastdist;

		float iwd = smoothlimit(invwaterdepth, 0.8);
		intensity *= iwd * 1.5;

		color += wavefoam * underground * intensity;
		// cliff foam
		color += (wavefoam * wavefoam) * (underground * 5.5 * (coast.r * coast.r * coast.r) * (coastdist * coastdist * coastdist * coastdist));
	}
	#endif

	return color;
}


vec4 GetReflection(vec3 normal, float angle)
{
 	vec4 reflColor = vec4(0.0, 0.0, 0.0, 0.0);

	#ifdef opt_reflection
	// we have to mirror the Y-axis
	reftexcoord  = vec2(reftexcoord.x, 1.0 - reftexcoord.y);
	reftexcoord += vec2(0.0, 3.0 * ScreenInverse.y) + normal.xz * 0.09 * ReflDistortion;

	reflColor.rgb = texture(reflection, reftexcoord).rgb;

		#ifdef opt_blurreflection
		vec2  v = BlurBase;
		float s = BlurExponent;
		reflColor.rgb += texture(reflection, reftexcoord.st + v).rgb;
		reflColor.rgb += texture(reflection, reftexcoord.st + v *s).rgb;
		reflColor.rgb += texture(reflection, reftexcoord.st + v *s*s).rgb;
		reflColor.rgb += texture(reflection, reftexcoord.st + v *s*s*s).rgb;
		reflColor.rgb += texture(reflection, reftexcoord.st + v *s*s*s*s).rgb;
		reflColor.rgb += texture(reflection, reftexcoord.st + v *s*s*s*s*s).rgb;
		reflColor.rgb += texture(reflection, reftexcoord.st + v *s*s*s*s*s*s).rgb;
		reflColor.rgb *= 0.125;
		#endif

	// fresnel term
	reflColor.a = FresnelMin + FresnelMax * pow(angle, FresnelPower);
	#endif

	return reflColor;
}






//////////////////////////////////////////////////
// MAIN()

void main()
{
	#ifdef dbg_coastmap
	fragColor = vec4(texture(coastmap, texCoord0.st).g);
	return;
	#endif

	// only rendered with blending iff !opt_refraction
	fragColor = vec4(0.0, 0.0, 0.0, 1.0);


	// GET WATERDEPTH
	//
	// depths.z := outside
	// depths.x := waterdepth
	// depths.y := invwaterdepth
	vec2 coast = vec2(0.0, 0.0);
	vec3 depths = GetWaterHeight(coast);

	float shallowScale = GetWaterDepthFromDepthBuffer(depths.x);


	// NORMALMAP
	vec3 octave;
	vec3 normal = GetNormal(octave);
	vec3 eyeDir = normalize(eyeVec);

	vec3 reflectDir   = reflect(normalize(-sunVec), normal);
	vec3 SunLow       = SunDir * vec3(1.0, 0.1, 1.0);

	float eyeNormalCos = dot(-eyeDir, normal);
	float angle = (1.0 - abs(eyeNormalCos));


	// SHADOW
	float shadowOcc = GetShadowOcclusion(worldPos + vec3(normal.x, 0.0, normal.z) * 10.0);


	// AMBIENT & DIFFUSE
	float diffuse     = pow(max(dot(normal, SunLow), 0.0), 3.0) * DiffuseFactor;
	float ambient     = smoothstep(-1.3, 0.0, eyeNormalCos) * AmbientFactor;
	float specular    = angle * pow(max(dot(reflectDir, eyeDir), 0.0), SpecularPower) * SpecularFactor * shallowScale;

	float surfaceMix  = (SurfaceColor.a + diffuse) * shallowScale;
	float refractDistortion = 60.0 * (1.0 - pow(gl_FragCoord.z, 80.0)) * shallowScale;

	// NB: DiffuseColor (water.diffuseColor) and DiffuseFactor (water.diffuseFactor) default to 1
	vec3 waterSurface = SurfaceColor.rgb + DiffuseColor * diffuse + vec3(ambient);

	// INFOTEX
	waterSurface += GetInfoTex(depths.z);


	{
		// REFRACTION
		#ifdef opt_refraction
			#ifdef opt_texrect
			vec3 refrColor = texture(refraction, screencoord + normal.xz * refractDistortion).rgb;
			#else
			vec3 refrColor = texture(refraction, screencoord + normal.xz * refractDistortion * ScreenInverse).rgb;
			#endif

			fragColor.rgb = mix(refrColor, waterSurface, 0.1 + surfaceMix);
		#else
			fragColor.rgb = waterSurface;
			fragColor.a   = surfaceMix + specular;
		#endif
	}

	// CAUSTICS
	if (depths.x > 0.0) {
		vec3 caust = texture(caustic, texCoord0.pq * 75.0).rgb;

		#ifdef opt_refraction
		float caustBlend = smoothstep(CausticRange, 0.0, abs(depths.x - CausticDepth));
		fragColor.rgb += caust * caustBlend * 0.08;
		#else
		fragColor.a *= min(depths.x * 4.0, 1.0);
		fragColor.rgb += caust * (1.0 - depths.x) * 0.6;
		#endif
	}

	{
		// SHORE WAVES
		fragColor.rgb += shadowOcc * GetShorewaves(coast, octave, depths.x, depths.y);
	}

	{
		// REFLECTION
		// Schlick's approx. for Fresnel term
		vec4 reflColor = GetReflection(normal, angle);
		fragColor.rgb = mix(fragColor.rgb, reflColor.rgb, reflColor.a * shallowScale);
	}

	{
		// SPECULAR
		fragColor.rgb += (shadowOcc * specular * SpecularColor);
	}

	{
		// FOG
		float fogRange = (fogParams.y - fogParams.x) * fogParams.z;
		float fogDepth = (fogCoord - fogParams.x * fogParams.z) / fogRange;
		// float fogDepth = (fogParams.y * fogParams.z - fogCoord) / fogRange;

		float fogFactor = 1.0 - clamp(fogDepth, 0.0, 1.0);
		// float fogFactor = clamp(fogDepth, 0.0, 1.0);

		fragColor.rgb = mix(fogColor.rgb, fragColor.rgb, fogFactor);
	}

	fragColor.rgb = pow(fragColor.rgb, vec3(gammaExponent));
}

