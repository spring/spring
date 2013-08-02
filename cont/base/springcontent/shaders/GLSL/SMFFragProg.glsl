#if (SMF_PARALLAX_MAPPING == 1)
#version 120
#endif

#define SSMF_UNCOMPRESSED_NORMALS 0
#define SMF_SHALLOW_WATER_DEPTH     (10.0                          )
#define SMF_SHALLOW_WATER_DEPTH_INV ( 1.0 / SMF_SHALLOW_WATER_DEPTH)
#define SMF_DETAILTEX_RES 0.02

uniform sampler2D diffuseTex;
uniform sampler2D normalsTex;
uniform sampler2D detailTex;
uniform sampler2D infoTex;
uniform vec2 normalTexGen;   // either 1.0/mapSize (when NPOT are supported) or 1.0/mapSizePO2
uniform vec2 specularTexGen; // 1.0/mapSize

#if (SMF_ARB_LIGHTING == 0)
	uniform sampler2D specularTex;
#endif

#if (HAVE_SHADOWS == 1)
uniform sampler2DShadow shadowTex;
uniform mat4 shadowMat;
uniform vec4 shadowParams;
#endif

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;
uniform vec3 groundSpecularColor;
uniform float groundShadowDensity;

#if (SMF_WATER_ABSORPTION == 1)
	uniform vec3 waterMinColor;
	uniform vec3 waterBaseColor;
	uniform vec3 waterAbsorbColor;
#endif

#if (SMF_DETAIL_TEXTURE_SPLATTING == 1)
	uniform sampler2D splatDetailTex;
	uniform sampler2D splatDistrTex;
	uniform vec4 splatTexMults;  // per-channel splat intensity multipliers
	uniform vec4 splatTexScales; // defaults to SMF_DETAILTEX_RES per channel
#endif


#if (SMF_SKY_REFLECTIONS == 1)
	uniform samplerCube skyReflectTex;
	uniform sampler2D skyReflectModTex;
#endif

#if (SMF_DETAIL_NORMALS == 1)
	uniform sampler2D detailNormalTex;
#endif

#if (SMF_LIGHT_EMISSION == 1)
	uniform sampler2D lightEmissionTex;
#endif

#if (SMF_PARALLAX_MAPPING == 1)
	uniform sampler2D parallaxHeightTex;
#endif

#if (HAVE_INFOTEX == 1)
uniform float infoTexIntensityMul;
uniform vec2 infoTexGen;     // 1.0/(pwr2map{x,z} * SQUARE_SIZE)
#endif

uniform vec4 lightDir;
uniform vec3 cameraPos;
varying vec3 halfDir;

varying float fogFactor;
varying vec4 vertexWorldPos;
varying vec2 diffuseTexCoords;



#if (SMF_PARALLAX_MAPPING == 1)
vec2 GetParallaxUVOffset(vec2 uv, vec3 dir) {
	vec4 texel = texture2D(parallaxHeightTex, uv);

	// RG: height in [ 0.0, 1.0] (256^2 strata)
	//  B: scale  in [ 0.0, 1.0] (256   strata), eg.  0.04 (~10.0/256.0)
	//  A: bias   in [-0.5, 0.5] (256   strata), eg. -0.02 (~75.0/256.0)
	//
	#define RMUL 65280.0 // 255 * 256
	#define GMUL   256.0
	#define HDIV 65536.0

	float heightValue  = (texel.r * RMUL + texel.g * GMUL) / HDIV;
	float heightScale  = texel.b;
	float heightBias   = texel.a - 0.5;
	float heightOffset = heightValue * heightScale + heightBias;

	return ((dir.xy / dir.z) * heightOffset);
}
#endif

vec3 GetFragmentNormal(vec2 uv) {
	vec3 normal;

	#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		normal = normalize(texture2D(normalsTex, uv).xyz);
	#else
		normal.xz = texture2D(normalsTex, uv).ra;
		normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	#endif

	return normal;
}

vec4 GetDetailTextureColor(vec2 uv) {
	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
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

vec4 GetShadeInt(float groundLightInt, float groundShadowCoeff, float groundDiffuseAlpha) {
	vec4 groundShadeInt;
	vec4 waterShadeInt;

	groundShadeInt.rgb = groundAmbientColor + groundDiffuseColor * (groundLightInt * groundShadowCoeff);
	groundShadeInt.rgb *= SMF_INTENSITY_MULT;

	#if (SMF_VOID_GROUND == 1)
	// assume the map(per)'s diffuse texture provides sensible alphas
	groundShadeInt.a = groundDiffuseAlpha;
	#else
	groundShadeInt.a = 1.0;
	#endif

	#if (SMF_WATER_ABSORPTION == 1)
	{
		float waterHeightAlpha = abs(vertexWorldPos.y) * SMF_SHALLOW_WATER_DEPTH_INV;
		float waterShadeDecay  = 0.2 + (waterHeightAlpha * 0.1);
		float vertexStepHeight = min(1023.0, -floor(vertexWorldPos.y));
		float waterLightInt    = min((groundLightInt + 0.2) * 2.0, 1.0);

		#if (SMF_VOID_WATER == 1)
		// cut out all underwater fragments indiscriminately
		waterShadeInt.a = float(vertexWorldPos.y >= 0.0);
		#else
		// allow voidground maps to create holes in the seabed
		waterShadeInt.a = groundShadeInt.a;
		#endif

		waterShadeInt.rgb = waterBaseColor.rgb - (waterAbsorbColor.rgb * vertexStepHeight);
		waterShadeInt.rgb = max(waterMinColor.rgb, waterShadeInt.rgb);
		waterShadeInt.rgb *= SMF_INTENSITY_MULT * waterLightInt;

		// make shadowed areas darker over deeper water
		waterShadeInt.rgb -= (waterShadeInt.rgb * waterShadeDecay * (1.0 - groundShadowCoeff));

		// "shallow" water, interpolate between groundShadeInt
		// and waterShadeInt (both are already cosine-weighted)
		if (vertexWorldPos.y > -SMF_SHALLOW_WATER_DEPTH) {
			waterShadeInt.rgb = mix(groundShadeInt.rgb, waterShadeInt.rgb, waterHeightAlpha);
		}
	}

	return mix(groundShadeInt, waterShadeInt, float(vertexWorldPos.y < 0.0));
	#else
	return groundShadeInt;
	#endif
}



void main() {
	vec2 diffTexCoords = diffuseTexCoords;
	vec2 specTexCoords = vertexWorldPos.xz * specularTexGen;
	vec2 normTexCoords = vertexWorldPos.xz * normalTexGen;

	// not calculated in the vertex shader to save varying components (OpenGL2.0 allows just 32)
	vec3 cameraDir = vertexWorldPos.xyz - cameraPos;
	vec3 normal = GetFragmentNormal(normTexCoords);

	#if (SMF_DETAIL_NORMALS == 1 || SMF_PARALLAX_MAPPING == 1)
		// detail-normals are (assumed to be) defined within STN space
		// (for a regular vertex normal equal to <0, 1, 0>, the S- and
		// T-tangents are aligned with Spring's +x and +z (!) axes)
		vec3 tTangent = cross(normal, vec3(-1.0, 0.0, 0.0));
		vec3 sTangent = cross(normal, tTangent);
		mat3 stnMatrix = mat3(sTangent, tTangent, normal);
	#endif


	#if (SMF_PARALLAX_MAPPING == 1)
	{
		// use specular-texture coordinates to index parallaxHeightTex
		// (ie. specularTex and parallaxHeightTex must have equal size)
		// cameraDir does not need to be normalized, x/z and y/z ratios
		// do not change
		vec2 uvOffset = GetParallaxUVOffset(specTexCoords, transpose(stnMatrix) * cameraDir);
		vec2 normTexSize = 1.0 / normalTexGen;
		vec2 specTexSize = 1.0 / specularTexGen;

		// scale the parallax offset since it is in spectex-space
		diffTexCoords += (uvOffset * (specTexSize / SMF_TEXSQUARE_SIZE));
		normTexCoords += (uvOffset * (specTexSize / normTexSize));
		specTexCoords += (uvOffset);

		normal = GetFragmentNormal(normTexCoords);
	}
	#endif


	#if (SMF_DETAIL_NORMALS == 1)
	{
		vec4 dtSample = texture2D(detailNormalTex, normTexCoords);
		vec3 dtNormal = (dtSample.xyz * 2.0) - 1.0;

		// convert dtNormal from TS to WS before mixing
		normal = normalize(mix(normal, stnMatrix * dtNormal, dtSample.a));
	}
	#endif


	float cosAngleDiffuse = clamp(dot(normalize(lightDir.xyz), normal), 0.0, 1.0);
	float cosAngleSpecular = clamp(dot(normalize(halfDir), normal), 0.0, 1.0);

	vec4 diffuseCol = texture2D(diffuseTex, diffTexCoords);
	vec4 detailCol = GetDetailTextureColor(specTexCoords);
	vec4 specularCol = vec4(0.5, 0.5, 0.5, 1.0);

	#if (SMF_SKY_REFLECTIONS == 1)
	{
		// cameraDir does not need to be normalized for reflect()
		vec3 reflectDir = reflect(cameraDir, normal);
		vec3 reflectCol = textureCube(skyReflectTex, gl_NormalMatrix * reflectDir).rgb;
		vec3 reflectMod = texture2D(skyReflectModTex, specTexCoords).rgb;

		diffuseCol.rgb = mix(diffuseCol.rgb, reflectCol, reflectMod);
	}
	#endif
	#if (HAVE_INFOTEX == 1)
		// increase contrast and brightness for the overlays
		// TODO: make the multiplier configurable by users?
		vec2 infoTexCoords = vertexWorldPos.xz * infoTexGen;
		diffuseCol.rgb += (texture2D(infoTex, infoTexCoords).rgb * infoTexIntensityMul);
		diffuseCol.rgb -= (vec3(0.5, 0.5, 0.5) * float(infoTexIntensityMul == 1.0));
	#endif



	float shadowCoeff = 1.0;

	#if (HAVE_SHADOWS == 1)
	{
		vec2 p17 = vec2(shadowParams.z, shadowParams.z);
		vec2 p18 = vec2(shadowParams.w, shadowParams.w);

		vec4 vertexShadowPos = shadowMat * vertexWorldPos;
			vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
			vertexShadowPos.st += shadowParams.xy;

		// same as ARB shader: shadowCoeff = 1 - (1 - shadowCoeff) * groundShadowDensity
		shadowCoeff = shadow2DProj(shadowTex, vertexShadowPos).r;
		shadowCoeff = mix(1.0, shadowCoeff, groundShadowDensity);
	}
	#endif

	{
		// GroundMaterialAmbientDiffuseColor * LightAmbientDiffuseColor
		vec4 shadeInt = GetShadeInt(cosAngleDiffuse, shadowCoeff, diffuseCol.a);

		gl_FragColor.rgb = (diffuseCol.rgb + detailCol.rgb) * shadeInt.rgb;
		gl_FragColor.a = shadeInt.a;
	}

	#if (SMF_LIGHT_EMISSION == 1)
	{
		vec4 lightEmissionCol = texture2D(lightEmissionTex, specTexCoords);
		vec3 scaledFragmentCol = gl_FragColor.rgb * (1.0 - lightEmissionCol.a);

		gl_FragColor.rgb = scaledFragmentCol + lightEmissionCol.rgb;
	}
	#endif

	#if (SMF_ARB_LIGHTING == 0)
		specularCol = texture2D(specularTex, specTexCoords);

		// sun specular lighting contribution
		float specularExp  = specularCol.a * 16.0;
		float specularPow  = pow(cosAngleSpecular, specularExp);

		vec3  specularInt  = specularCol.rgb * specularPow;
		      specularInt *= shadowCoeff;

		// no need to multiply by groundSpecularColor anymore
		gl_FragColor.rgb += specularInt;
	#endif

	#if (MAX_DYNAMIC_MAP_LIGHTS > 0)
	for (int i = 0; i < MAX_DYNAMIC_MAP_LIGHTS; i++) {
		vec3 lightVec = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].position.xyz - vertexWorldPos.xyz;
		vec3 halfVec = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].halfVector.xyz;

		float lightRadius = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].constantAttenuation;
		float lightDistance = length(lightVec);
		float lightScale = (lightDistance > lightRadius)? 0.0: 1.0;
		float lightCosAngDiff = clamp(dot(normal, lightVec / lightDistance), 0.0, 1.0);
		float lightCosAngSpec = clamp(dot(normal, normalize(halfVec)), 0.0, 1.0);
		#ifdef OGL_SPEC_ATTENUATION
		float lightAttenuation =
			(gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].constantAttenuation) +
			(gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].linearAttenuation * lightDistance) +
			(gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].quadraticAttenuation * lightDistance * lightDistance);

		lightAttenuation = 1.0 / max(lightAttenuation, 1.0);
		#else
		float lightAttenuation = 1.0 - min(1.0, ((lightDistance * lightDistance) / (lightRadius * lightRadius)));
		#endif

		float vectorDot = dot((-lightVec / lightDistance), gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].spotDirection);
		float cutoffDot = gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].spotCosCutoff;

		#if (SMF_ARB_LIGHTING == 0)
		float lightSpecularPow = max(0.0, pow(lightCosAngSpec, specularExp));
		#else
		float lightSpecularPow = 0.0;
		#endif

		lightScale *= ((vectorDot < cutoffDot)? 0.0: 1.0);

		gl_FragColor.rgb += (lightScale *                                       gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].ambient.rgb);
		gl_FragColor.rgb += (lightScale * lightAttenuation * (diffuseCol.rgb  * gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].diffuse.rgb * lightCosAngDiff));
		gl_FragColor.rgb += (lightScale * lightAttenuation * (specularCol.rgb * gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].specular.rgb * lightSpecularPow));
	}
	#endif

	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor);
}

