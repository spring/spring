#version 410 core

// #define use_normalmapping
// #define flip_normalmap

const float DEGREES_TO_RADIANS = 3.141592653589793 / 180.0;

uniform sampler2D diffuseTex;
uniform sampler2D shadingTex;
uniform samplerCube reflectTex;
#ifdef use_normalmapping
uniform sampler2D normalMap;
#endif

uniform vec3 sunDir;
uniform vec3 sunDiffuse; // model
uniform vec3 sunAmbient; // model
uniform vec3 sunSpecular; // model

#if (USE_SHADOWS == 1)
uniform sampler2DShadow shadowTex;
uniform mat4 shadowMatrix;
uniform vec4 shadowParams;
uniform float shadowDensity;
#endif
uniform float specularExponent;
uniform float gammaExponent;

uniform vec4 fogColor;
// in opaque passes tc.a is always 1.0 [all objects], and alphaPass is 0.0
// in alpha passes tc.a is either one of alphaValues.xyzw [for units] *or*
// contains a distance fading factor [for features], and alphaPass is 1.0
// texture alpha-masking is done in both passes
uniform vec4 teamColor;
uniform vec4 nanoColor;
// uniform float alphaPass;

uniform vec4 alphaTestCtrl;

// fwdDynLights[i] := {pos, dir, diffuse, specular, ambient, {fov, radius, -, -}}
uniform vec4 fwdDynLights[MAX_LIGHT_UNIFORM_VECS];


in vec4 worldPos;
in vec3 cameraDir;

in vec2 texCoord0;
// in vec2 texCoord1;

in float fogFactor;

#ifdef use_normalmapping
in mat3 tbnMatrix;
#else
in vec3 wsNormalVec;
#endif

#if (DEFERRED_MODE == 0)
layout(location = 0) out vec4 fragColor;
#else
layout(location = 0) out vec4 fragData[MDL_FRAGDATA_COUNT];
#endif



float GetShadowCoeff(float zBias) {
	#if (USE_SHADOWS == 1)
	vec4 vertexShadowPos = shadowMatrix * worldPos;
		vertexShadowPos.z  += zBias;

	return mix(1.0, textureProj(shadowTex, vertexShadowPos), shadowDensity);
	#endif
	return 1.0;
}

vec3 DynamicLighting(vec3 wsNormal, vec3 camDir, vec3 diffuseColor, vec4 specularColor) {
	vec3 light = vec3(0.0);

	#if (NUM_DYNAMIC_MODEL_LIGHTS > 0)
	for (int i = 0; i < NUM_DYNAMIC_MODEL_LIGHTS; i++) {
		int j = i * 6;

		vec4 wsLightPos = fwdDynLights[j + 0];
		vec4 wsLightDir = fwdDynLights[j + 1];

		vec4 lightDiffColor = fwdDynLights[j + 2];
		vec4 lightSpecColor = fwdDynLights[j + 3];
		vec4 lightAmbiColor = fwdDynLights[j + 4];

		vec3 wsLightVec = normalize(wsLightPos.xyz - worldPos.xyz);
		vec3 wsHalfVec = normalize(camDir + wsLightVec);

		float lightAngle    = fwdDynLights[j + 5].x; // fov
		float lightRadius   = fwdDynLights[j + 5].y; // or const. atten.
		float lightDistance = dot(wsLightVec, wsLightPos.xyz - worldPos.xyz);


		float lightCosAngleDiff = clamp(dot(wsNormal, wsLightVec), 0.0, 1.0);
		float lightCosAngleSpec = clamp(dot(wsNormal, wsHalfVec), 0.001, 1.0);

		#ifdef OGL_SPEC_ATTENUATION
		// infinite falloff
		float cLightAtten = fwdDynLights[j + 5].y;
		float lLightAtten = fwdDynLights[j + 5].z;
		float qLightAtten = fwdDynLights[j + 5].w;
		float  lightAtten = cLightAtten + lLightAtten * lightDistance + qLightAtten * lightDistance * lightDistance;
		float  lightConst = 1.0;

		lightAtten = 1.0 / max(lightAtten, 1.0);
		#else
		float lightConst = float(lightDistance <= lightRadius);
		float lightAtten = 1.0 - min(1.0, ((lightDistance * lightDistance) / (lightRadius * lightRadius)));
		#endif

		float lightSpecularPow = max(0.0, pow(lightCosAngleSpec, specularColor.a));

		float vectorCosAngle = dot(-wsLightVec, wsLightDir.xyz);
		float cutoffCosAngle = cos(lightAngle * DEGREES_TO_RADIANS);

		lightConst *= float(vectorCosAngle >= cutoffCosAngle);

		light += (lightConst *                                   lightAmbiColor.rgb                     );
		light += (lightConst * lightAtten * ( diffuseColor.rgb * lightDiffColor.rgb * lightCosAngleDiff));
		light += (lightConst * lightAtten * (specularColor.rgb * lightSpecColor.rgb * lightSpecularPow ));
	}
	#endif

	return light;
}

void main(void)
{
#ifdef use_normalmapping
	vec2 tc = texCoord0;
	#ifdef flip_normalmap
		tc.t = 1.0 - tc.t;
	#endif

	vec3 tsNormal = normalize((texture(normalMap, tc).xyz - 0.5) * 2.0);
	vec3 wsNormal = tbnMatrix * tsNormal;
#else
	vec3 wsNormal = normalize(wsNormalVec);
#endif

	vec3 reflectDir = reflect(cameraDir, wsNormal);

	vec3 sunLightColor = max(dot(wsNormal, sunDir), 0.0) * sunDiffuse + sunAmbient;

	vec4 diffuseColor = texture(diffuseTex, texCoord0);
	vec4 shadingColor = texture(shadingTex, texCoord0);

	vec3 specularColor = sunSpecular * pow(max(0.001, dot(wsNormal, normalize(sunDir + cameraDir * -1.0))), specularExponent);
	vec3  reflectColor = texture(reflectTex,  reflectDir).rgb;


	float shadow = GetShadowCoeff(-0.00005);
	float alpha = teamColor.a * shadingColor.a; // apply one-bit mask

	#if (DEFERRED_MODE == 0)
	float alphaTestGreater = float(alpha > alphaTestCtrl.x) * alphaTestCtrl.y;
	float alphaTestSmaller = float(alpha < alphaTestCtrl.x) * alphaTestCtrl.z;

	if ((alphaTestGreater + alphaTestSmaller + alphaTestCtrl.w) == 0.0)
		discard;
	#endif

	specularColor *= (shadingColor.g * 4.0);
	// no highlights if in shadow; decrease light to ambient level
	specularColor *= shadow;
	sunLightColor = mix(sunAmbient, sunLightColor, shadow);


	reflectColor  = mix(sunLightColor, reflectColor, shadingColor.g); // reflection
	reflectColor += shadingColor.rrr; // self-illum

	#if (DEFERRED_MODE == 0)
	fragColor     = diffuseColor;
	fragColor.rgb = mix(fragColor.rgb, teamColor.rgb, fragColor.a); // teamcolor
	fragColor.rgb = fragColor.rgb * reflectColor + specularColor;
	#endif

	#if (DEFERRED_MODE == 0)
	fragColor.rgb += DynamicLighting(wsNormal, cameraDir * -1.0, diffuseColor.rgb, vec4(specularColor, 4.0));
	#endif

	#if (DEFERRED_MODE == 1)
	fragData[GBUFFER_NORMTEX_IDX] = vec4((wsNormal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
	fragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(                 diffuseColor.rgb, teamColor.rgb, diffuseColor.a), alpha);
	fragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(fragData[GBUFFER_DIFFTEX_IDX].rgb, nanoColor.rgb, nanoColor.a), alpha);
	// do not premultiply reflection, leave it to the deferred lighting pass
	// fragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(diffuseColor.rgb, teamColor.rgb, diffuseColor.a) * reflectColor, alpha);
	// allows standard-lighting reconstruction by lazy LuaMaterials using us
	fragData[GBUFFER_SPECTEX_IDX] = vec4(shadingColor.rgb, alpha);
	fragData[GBUFFER_EMITTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	fragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	#else
	fragColor.rgb = mix(fogColor.rgb, fragColor.rgb, fogFactor); // fog
	fragColor.rgb = mix(fragColor.rgb, nanoColor.rgb, nanoColor.a); // wireframe or polygon color
	fragColor.rgb = pow(fragColor.rgb, vec3(gammaExponent));
	fragColor.a   = alpha;
	#endif
}

