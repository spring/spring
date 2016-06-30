#version 130

#ifdef NOSPRING
	#define SMF_INTENSITY_MULT (210.0 / 255.0)
	#define SMF_TEXSQUARE_SIZE 1024.0
	#define MAX_DYNAMIC_MAP_LIGHTS 4
	#define BASE_DYNAMIC_MAP_LIGHT 0
	#define GBUFFER_NORMTEX_IDX 0
	#define GBUFFER_DIFFTEX_IDX 1
	#define GBUFFER_SPECTEX_IDX 2
	#define GBUFFER_EMITTEX_IDX 3
	#define GBUFFER_MISCTEX_IDX 4
#endif


/***********************************************************************/
// Consts

const float SMF_SHALLOW_WATER_DEPTH     = 10.0;
const float SMF_SHALLOW_WATER_DEPTH_INV = 1.0 / SMF_SHALLOW_WATER_DEPTH;
const float SMF_DETAILTEX_RES           = 0.02;


/***********************************************************************/
// Uniforms + Varyings

uniform sampler2D diffuseTex;
uniform sampler2D normalsTex;
uniform sampler2D detailTex;
uniform vec2 normalTexGen;   // either 1.0/mapSize (when NPOT are supported) or 1.0/mapSizePO2
uniform vec2 specularTexGen; // 1.0/mapSize

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;
uniform vec3 groundSpecularColor;
uniform float groundSpecularExponent;
uniform float groundShadowDensity;

uniform vec2 mapHeights; // min & max height on the map

uniform vec4 lightDir;
uniform vec3 cameraPos;

varying vec3 halfDir;
varying float fogFactor;
varying vec4 vertexWorldPos;
varying vec2 diffuseTexCoords;

#ifdef HAVE_INFOTEX
	uniform sampler2D infoTex;
	uniform float infoTexIntensityMul;
	uniform vec2 infoTexGen;     // 1.0/(pwr2map{x,z} * SQUARE_SIZE)
#endif

#ifdef SMF_SPECULAR_LIGHTING
	uniform sampler2D specularTex;
#endif

#ifdef HAVE_SHADOWS
	uniform sampler2DShadow shadowTex;
	uniform mat4 shadowMat;
	uniform vec4 shadowParams;
#endif

#ifdef SMF_WATER_ABSORPTION
	uniform vec3 waterMinColor;
	uniform vec3 waterBaseColor;
	uniform vec3 waterAbsorbColor;
#endif

#if defined(SMF_DETAIL_TEXTURE_SPLATTING) && !defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING)
	uniform sampler2D splatDetailTex;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif

#ifdef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
	uniform sampler2D splatDetailNormalTex1;
	uniform sampler2D splatDetailNormalTex2;
	uniform sampler2D splatDetailNormalTex3;
	uniform sampler2D splatDetailNormalTex4;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif
#ifdef SMF_SKY_REFLECTIONS
	uniform samplerCube skyReflectTex;
	uniform sampler2D skyReflectModTex;
#endif

#ifdef SMF_BLEND_NORMALS
	uniform sampler2D blendNormalsTex;
#endif

#ifdef SMF_LIGHT_EMISSION
	uniform sampler2D lightEmissionTex;
#endif

#ifdef SMF_PARALLAX_MAPPING
	uniform sampler2D parallaxHeightTex;
#endif


/***********************************************************************/
// Helper functions

#ifdef SMF_PARALLAX_MAPPING
vec2 GetParallaxUVOffset(vec2 uv, vec3 dir) {
	vec4 texel = texture2D(parallaxHeightTex, uv);

	// RG: height in [ 0.0, 1.0] (256^2 strata)
	//  B: scale  in [ 0.0, 1.0] (256   strata), eg.  0.04 (~10.0/256.0)
	//  A: bias   in [-0.5, 0.5] (256   strata), eg. -0.02 (~75.0/256.0)
	//
	const float RMUL = 255.0 * 256.0;
	const float GMUL = 256.0;
	const float HDIV = 65536.0;

	float heightValue  = dot(texel.rg, vec2(RMUL, GMUL)) / HDIV;
	float heightScale  = texel.b;
	float heightBias   = texel.a - 0.5;
	float heightOffset = heightValue * heightScale + heightBias;

	return ((dir.xy / dir.z) * heightOffset);
}
#endif

vec3 GetFragmentNormal(vec2 uv) {
	vec3 normal;
#ifdef SSMF_UNCOMPRESSED_NORMALS
	normal = normalize(texture2D(normalsTex, uv).xyz);
#else
	normal.xz = texture2D(normalsTex, uv).ra;
	normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
#endif
	return normal;
}


#ifndef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
vec4 GetDetailTextureColor(vec2 uv) {
	#ifndef SMF_DETAIL_TEXTURE_SPLATTING
		vec2 detailTexCoord = vertexWorldPos.xz * vec2(SMF_DETAILTEX_RES);
		vec4 detailCol = (texture2D(detailTex, detailTexCoord) * 2.0) - 1.0;
	#else
		vec4 splatTexCoord0 = vertexWorldPos.xzxz * splatTexScales.rrgg;
		vec4 splatTexCoord1 = vertexWorldPos.xzxz * splatTexScales.bbaa;
		vec4 splatDetails;
			splatDetails.r = texture2D(splatDetailTex, splatTexCoord0.st).r;
			splatDetails.g = texture2D(splatDetailTex, splatTexCoord0.pq).g;
			splatDetails.b = texture2D(splatDetailTex, splatTexCoord1.st).b;
			splatDetails.a = texture2D(splatDetailTex, splatTexCoord1.pq).a;
			splatDetails   = (splatDetails * 2.0) - 1.0;
		vec4 splatCofac = texture2D(splatDistrTex, uv) * splatTexMults;
		vec4 detailCol = vec4(dot(splatDetails, splatCofac));
	#endif
	return detailCol;
}
#else // SMF_DETAIL_NORMAL_TEXTURE_SPLATTING is defined
vec4 GetSplatDetailTextureNormal(vec2 uv, out vec2 splatDetailStrength) {
	vec4 splatTexCoord0 = vertexWorldPos.xzxz * splatTexScales.rrgg;
	vec4 splatTexCoord1 = vertexWorldPos.xzxz * splatTexScales.bbaa;
	vec4 splatCofac = texture2D(splatDistrTex, uv) * splatTexMults;

	// dot with 1's to sum up the splat distribution weights
	splatDetailStrength.x = min(1.0, dot(splatCofac, vec4(1.0)));

	vec4 splatDetailNormal;
		splatDetailNormal  = ((texture2D(splatDetailNormalTex1, splatTexCoord0.st) * 2.0 - 1.0) * splatCofac.r);
		splatDetailNormal += ((texture2D(splatDetailNormalTex2, splatTexCoord0.pq) * 2.0 - 1.0) * splatCofac.g);
		splatDetailNormal += ((texture2D(splatDetailNormalTex3, splatTexCoord1.st) * 2.0 - 1.0) * splatCofac.b);
		splatDetailNormal += ((texture2D(splatDetailNormalTex4, splatTexCoord1.pq) * 2.0 - 1.0) * splatCofac.a);

	// note: y=0.01 (pointing up) in case all splat-cofacs are zero
	splatDetailNormal.y = max(splatDetailNormal.y, 0.01);

	#ifdef SMF_DETAIL_NORMAL_DIFFUSE_ALPHA
		splatDetailStrength.y = clamp(splatDetailNormal.a, -1.0, 1.0);
	#endif

	// note: .xyz is intentionally not normalized here
	// (the normal will be rotated to worldspace first)
	return splatDetailNormal;
}
#endif


vec4 GetShadeInt(float groundLightInt, float groundShadowCoeff, float groundDiffuseAlpha) {
	vec4 groundShadeInt = vec4(0.0, 0.0, 0.0, 1.0);

	groundShadeInt.rgb = groundAmbientColor + groundDiffuseColor * (groundLightInt * groundShadowCoeff);
	groundShadeInt.rgb *= vec3(SMF_INTENSITY_MULT);

#ifdef SMF_VOID_WATER
	// cut out all underwater fragments indiscriminately
	groundShadeInt.a = float(vertexWorldPos.y >= 0.0);
#endif

#ifdef SMF_VOID_GROUND
	// assume the map(per)'s diffuse texture provides sensible alphas
	// note that voidground overrides voidwater if *both* are enabled
	// (limiting it to just above-water fragments would be arbitrary)
	groundShadeInt.a = groundDiffuseAlpha;
#endif

#ifdef SMF_WATER_ABSORPTION
	// use alpha of groundShadeInt cause:
	// allow voidground maps to create holes in the seabed
	// (SMF_WATER_ABSORPTION == 1 implies voidwater is not
	// enabled but says nothing about the voidground state)
	vec4 waterShadeInt = vec4(waterBaseColor.rgb, groundShadeInt.a);
	if (mapHeights.x <= 0.0) {
		float waterShadeAlpha  = abs(vertexWorldPos.y) * SMF_SHALLOW_WATER_DEPTH_INV;
		float waterShadeDecay  = 0.2 + (waterShadeAlpha * 0.1);
		float vertexStepHeight = min(1023.0, -vertexWorldPos.y);
		float waterLightInt    = min(groundLightInt * 2.0 + 0.4, 1.0);

		// vertex below shallow water depth --> alpha=1
		// vertex above shallow water depth --> alpha=waterShadeAlpha
		waterShadeAlpha = min(1.0, waterShadeAlpha + float(vertexWorldPos.y <= -SMF_SHALLOW_WATER_DEPTH));

		waterShadeInt.rgb -= (waterAbsorbColor.rgb * vertexStepHeight);
		waterShadeInt.rgb  = max(waterMinColor.rgb, waterShadeInt.rgb);
		waterShadeInt.rgb *= vec3(SMF_INTENSITY_MULT * waterLightInt);

		// make shadowed areas darker over deeper water
		waterShadeInt.rgb *= (1.0 - waterShadeDecay * (1.0 - groundShadowCoeff));

		// if depth is greater than _SHALLOW_ depth, select waterShadeInt
		// otherwise interpolate between groundShadeInt and waterShadeInt
		// (both are already cosine-weighted)
		waterShadeInt.rgb = mix(groundShadeInt.rgb, waterShadeInt.rgb, waterShadeAlpha);
	}
	return mix(groundShadeInt, waterShadeInt, float(vertexWorldPos.y < 0.0));
#else
	return groundShadeInt;
#endif
}


vec3 DynamicLighting(vec3 normal, vec3 diffuseCol, vec3 specularCol, float specularExp) {
	vec3 light = vec3(0.0);

	#ifndef SMF_SPECULAR_LIGHTING
		// non-zero default specularity on non-SSMF maps
		specularCol = vec3(0.5, 0.5, 0.5);
	#endif

	for (int i = 0; i < MAX_DYNAMIC_MAP_LIGHTS; i++) {
		vec3 lightVec = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].position.xyz - vertexWorldPos.xyz;
		vec3 halfVec = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].halfVector.xyz;

		float lightRadius = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].constantAttenuation;
		float lightDistance = length(lightVec);
		float lightScale = float(lightDistance <= lightRadius);
		float lightCosAngDiff = clamp(dot(normal, lightVec / lightDistance), 0.0, 1.0);
		//clamp lightCosAngSpec from 0.001 because this will later be in a power function
		//results are undefined if x==0 or if x==0 and y==0.
		float lightCosAngSpec = clamp(dot(normal, normalize(halfVec)), 0.001, 1.0);
	#ifdef OGL_SPEC_ATTENUATION
		float lightAttenuation =
			(gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].constantAttenuation) +
			(gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].linearAttenuation * lightDistance) +
			(gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].quadraticAttenuation * lightDistance * lightDistance);

		lightAttenuation = 1.0 / max(lightAttenuation, 1.0);
	#else
		float lightAttenuation = 1.0 - min(1.0, ((lightDistance * lightDistance) / (lightRadius * lightRadius)));
	#endif

		float vectorDot = -dot((lightVec / lightDistance), gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].spotDirection);
		float cutoffDot = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].spotCosCutoff;

		float lightSpecularPow = 0.0;
	#ifdef SMF_SPECULAR_LIGHTING
		lightSpecularPow = max(0.0, pow(lightCosAngSpec, specularExp));
	#endif

		lightScale *= float(vectorDot >= cutoffDot);

		light += (lightScale *                                       gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].ambient.rgb);
		light += (lightScale * lightAttenuation * (diffuseCol.rgb *  gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].diffuse.rgb * lightCosAngDiff));
		light += (lightScale * lightAttenuation * (specularCol.rgb * gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].specular.rgb * lightSpecularPow));
	}

	return light;
}

/***********************************************************************/
// main()

void main() {
	vec2 diffTexCoords = diffuseTexCoords;
	vec2 specTexCoords = vertexWorldPos.xz * specularTexGen;
	vec2 normTexCoords = vertexWorldPos.xz * normalTexGen;

	// not calculated in the vertex shader to save varying components (OpenGL2.0 allows just 32)
	vec3 cameraDir = vertexWorldPos.xyz - cameraPos;
	vec3 normal = GetFragmentNormal(normTexCoords);

	#if defined(SMF_BLEND_NORMALS) || defined(SMF_PARALLAX_MAPPING) || defined(SMF_DETAIL_NORMAL_TEXTURE_SPLATTING)
		// detail-normals are (assumed to be) defined within STN space
		// (for a regular vertex normal equal to <0, 1, 0>, the S- and
		// T-tangents are aligned with Spring's +x and +z (!) axes)
		vec3 tTangent = normalize(cross(normal, vec3(-1.0, 0.0, 0.0)));
		vec3 sTangent = cross(normal, tTangent);
		mat3 stnMatrix = mat3(sTangent, tTangent, normal);
	#endif


	#ifdef SMF_PARALLAX_MAPPING
	{
		// use specular-texture coordinates to index parallaxHeightTex
		// (ie. specularTex and parallaxHeightTex must have equal size)
		// cameraDir does not need to be normalized, x/z and y/z ratios
		// do not change
		vec2 uvOffset = GetParallaxUVOffset(specTexCoords, transpose(stnMatrix) * cameraDir);

		// scale the parallax offset since it is in spectex-space
		diffTexCoords += (uvOffset / (SMF_TEXSQUARE_SIZE * specularTexGen));
		normTexCoords += (uvOffset * (normalTexGen / specularTexGen));
		specTexCoords += (uvOffset);

		normal = GetFragmentNormal(normTexCoords);
	}
	#endif

	#ifdef SMF_BLEND_NORMALS
	{
		vec4 dtSample = texture2D(blendNormalsTex, normTexCoords);
		vec3 dtNormal = (dtSample.xyz * 2.0) - 1.0;

		// convert dtNormal from TS to WS before mixing
		normal = normalize(mix(normal, stnMatrix * dtNormal, dtSample.a));
	}
	#endif


	vec4 detailCol;

	#ifndef SMF_DETAIL_NORMAL_TEXTURE_SPLATTING
	{
		detailCol = GetDetailTextureColor(specTexCoords);
	}
	#else
	{
		// x-component modulates mixing of normals
		// y-component contains the detail color (splatDetailNormal.a if SMF_DETAIL_NORMAL_DIFFUSE_ALPHA)
		vec2 splatDetailStrength = vec2(0.0, 0.0);

		// note: splatDetailStrength is an OUT-param
		vec4 splatDetailNormal = GetSplatDetailTextureNormal(specTexCoords, splatDetailStrength);

		detailCol = vec4(splatDetailStrength.y);

		// convert the splat detail normal to worldspace,
		// then mix it with the regular one (note: needs
		// another normalization?)
		normal = mix(normal, normalize(stnMatrix * splatDetailNormal.xyz), splatDetailStrength.x);
	}
	#endif


#ifndef DEFERRED_MODE
	float cosAngleDiffuse = clamp(dot(lightDir.xyz, normal), 0.0, 1.0);
	float cosAngleSpecular = clamp(dot(normalize(halfDir), normal), 0.001, 1.0);
#endif

	vec4 diffuseCol = texture2D(diffuseTex, diffTexCoords);
	vec4 specularCol = vec4(0.0, 0.0, 0.0, 1.0);
	vec4 emissionCol = vec4(0.0, 0.0, 0.0, 0.0);

	#if !defined(DEFERRED_MODE) && defined(SMF_SKY_REFLECTIONS)
	{
		// cameraDir does not need to be normalized for reflect()
		vec3 reflectDir = reflect(cameraDir, normal);
		vec3 reflectCol = textureCube(skyReflectTex, gl_NormalMatrix * reflectDir).rgb;
		vec3 reflectMod = texture2D(skyReflectModTex, specTexCoords).rgb;

		diffuseCol.rgb = mix(diffuseCol.rgb, reflectCol, reflectMod);
	}
	#endif
	#if !defined(DEFERRED_MODE) && defined(HAVE_INFOTEX)
	{
		// increase contrast and brightness for the overlays
		// TODO: make the multiplier configurable by users?
		vec2 infoTexCoords = vertexWorldPos.xz * infoTexGen;
		diffuseCol.rgb += (texture2D(infoTex, infoTexCoords).rgb * infoTexIntensityMul);
		diffuseCol.rgb -= (vec3(0.5, 0.5, 0.5) * float(infoTexIntensityMul == 1.0));
	}
	#endif



	float shadowCoeff = 1.0;

	#if !defined(DEFERRED_MODE) && defined(HAVE_SHADOWS)
	{
		vec2 p17 = shadowParams.zz;
		vec2 p18 = shadowParams.ww;

		vec4 vertexShadowPos = shadowMat * vertexWorldPos;
			vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
			vertexShadowPos.st += shadowParams.xy;

		// same as ARB shader: shadowCoeff = 1 - (1 - shadowCoeff) * groundShadowDensity
		shadowCoeff = shadow2DProj(shadowTex, vertexShadowPos).r;
		shadowCoeff = mix(1.0, shadowCoeff, groundShadowDensity);
	}
	#endif

	#ifndef DEFERRED_MODE
	{
		// GroundMaterialAmbientDiffuseColor * LightAmbientDiffuseColor
		vec4 shadeInt = GetShadeInt(cosAngleDiffuse, shadowCoeff, diffuseCol.a);

		gl_FragColor.rgb = (diffuseCol.rgb + detailCol.rgb) * shadeInt.rgb;
		gl_FragColor.a = shadeInt.a;
	}
	#endif

	#ifdef SMF_LIGHT_EMISSION
	{
		// apply self-illumination aka. glow, not masked by shadows
		emissionCol = texture2D(lightEmissionTex, specTexCoords);

		#ifndef DEFERRED_MODE
		gl_FragColor.rgb = gl_FragColor.rgb * (1.0 - emissionCol.a) + emissionCol.rgb;
		#endif
	}
	#endif

#ifdef SMF_SPECULAR_LIGHTING
	specularCol = texture2D(specularTex, specTexCoords);
#else
	specularCol = vec4(groundSpecularColor, 1.0);
#endif

	#ifndef DEFERRED_MODE
		// sun specular lighting contribution
		#ifdef SMF_SPECULAR_LIGHTING
			float specularExp  = specularCol.a * 16.0;
		#else
			float specularExp  = groundSpecularExponent;
		#endif


		float specularPow  = pow(cosAngleSpecular, specularExp);

		vec3  specularInt  = specularCol.rgb * specularPow;
		      specularInt *= shadowCoeff;

		gl_FragColor.rgb += specularInt;

		#if (MAX_DYNAMIC_MAP_LIGHTS > 0)
			gl_FragColor.rgb += DynamicLighting(normal, diffuseCol.rgb, specularCol.rgb, specularExp);
		#endif
	#endif


#ifdef DEFERRED_MODE
	gl_FragData[GBUFFER_NORMTEX_IDX] = vec4((normal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
	gl_FragData[GBUFFER_DIFFTEX_IDX] = diffuseCol + detailCol;
	gl_FragData[GBUFFER_SPECTEX_IDX] = specularCol;
	gl_FragData[GBUFFER_EMITTEX_IDX] = emissionCol;
	gl_FragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);

	// linearly transform the eye-space depths, might be more useful?
	// gl_FragDepth = gl_FragCoord.z / gl_FragCoord.w;
#else
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor);
#endif
}

