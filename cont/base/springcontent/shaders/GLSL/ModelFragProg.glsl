#version 410 core

// #define use_normalmapping
// #define flip_normalmap

uniform sampler2D diffuseTex;
uniform sampler2D shadingTex;
uniform samplerCube specularTex;
uniform samplerCube reflectTex;
#ifdef use_normalmapping
uniform sampler2D normalMap;
#endif

uniform vec3 sunDir;
uniform vec3 sunDiffuse;
uniform vec3 sunAmbient;

#if (USE_SHADOWS == 1)
uniform sampler2DShadow shadowTex;
uniform mat4 shadowMatrix;
uniform vec4 shadowParams;
uniform float shadowDensity;
#endif

uniform vec4 fogColor;
// in opaque passes tc.a is always 1.0 [all objects], and alphaPass is 0.0
// in alpha passes tc.a is either one of alphaValues.xyzw [for units] *or*
// contains a distance fading factor [for features], and alphaPass is 1.0
// texture alpha-masking is done in both passes
uniform vec4 teamColor;
uniform vec4 nanoColor;
// uniform float alphaPass;


in vec4 worldPos;
in vec3 cameraDir;

in vec2 texCoord0;
// in vec2 texCoord1;

in float fogFactor;

#ifdef use_normalmapping
in mat3 tbnMatrix;
#else
in vec3 normalVec;
#endif

#if (DEFERRED_MODE == 0)
layout(location = 0) out vec4 fragColor;
#else
layout(location = 0) out vec4 fragData[MDL_FRAGDATA_COUNT];
#endif



float GetShadowCoeff(float zBias) {
	#if (USE_SHADOWS == 1)
	vec4 vertexShadowPos = shadowMatrix * worldPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;
		vertexShadowPos.z  += zBias;

	return mix(1.0, textureProj(shadowTex, vertexShadowPos), shadowDensity);
	#endif
	return 1.0;
}

vec3 DynamicLighting(vec3 normal, vec3 diffuse, vec3 specular) {
	vec3 rgb = vec3(0.0);

	#if 0
	for (int i = 0; i < MAX_DYNAMIC_MODEL_LIGHTS; i++) {
		vec3 lightVec = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].position.xyz - worldPos.xyz;
		vec3 halfVec = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].halfVector.xyz;

		float lightRadius   = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].constantAttenuation;
		float lightDistance = length(lightVec);
		float lightScale    = float(lightDistance <= lightRadius);

		float lightCosAngDiff = clamp(dot(normal, lightVec / lightDistance), 0.0, 1.0);
		float lightCosAngSpec = clamp(dot(normal, normalize(halfVec)), 0.0, 1.0);
		#ifdef OGL_SPEC_ATTENUATION
		float lightAttenuation =
			(gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].constantAttenuation) +
			(gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].linearAttenuation * lightDistance) +
			(gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].quadraticAttenuation * lightDistance * lightDistance);

		lightAttenuation = 1.0 / max(lightAttenuation, 1.0);
		#else
		float lightAttenuation = 1.0 - min(1.0, ((lightDistance * lightDistance) / (lightRadius * lightRadius)));
		#endif

		float vectorDot = dot((-lightVec / lightDistance), gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].spotDirection);
		float cutoffDot = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].spotCosCutoff;

		lightScale *= float(vectorDot >= cutoffDot);

		rgb += (lightScale *                                  gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].ambient.rgb);
		rgb += (lightScale * lightAttenuation * (diffuse.rgb * gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].diffuse.rgb * lightCosAngDiff));
		rgb += (lightScale * lightAttenuation * (specular.rgb * gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].specular.rgb * pow(lightCosAngSpec, 4.0)));
	}
	#endif

	return rgb;
}

void main(void)
{
#ifdef use_normalmapping
	vec2 tc = texCoord0;
	#ifdef flip_normalmap
		tc.t = 1.0 - tc.t;
	#endif

	vec3 nvTS  = normalize((texture(normalMap, tc).xyz - 0.5) * 2.0);
	vec3 normal = tbnMatrix * nvTS;
#else
	vec3 normal = normalize(normalVec);
#endif

	vec3 light = max(dot(normal, sunDir), 0.0) * sunDiffuse + sunAmbient;

	vec4 diffuse     = texture(diffuseTex, texCoord0);
	vec4 extraColor  = texture(shadingTex, texCoord0);

	vec3 reflectDir = reflect(cameraDir, normal);
	vec3 specular   = texture(specularTex, reflectDir).rgb * extraColor.g * 4.0;
	vec3 reflection = texture(reflectTex,  reflectDir).rgb;


	float shadow = GetShadowCoeff(-0.00005);
	float alpha = teamColor.a * extraColor.a; // apply one-bit mask

	// no highlights if in shadow; decrease light to ambient level
	specular *= shadow;
	light = mix(sunAmbient, light, shadow);


	reflection  = mix(light, reflection, extraColor.g); // reflection
	reflection += extraColor.rrr; // self-illum

	#if (DEFERRED_MODE == 0)
	fragColor     = diffuse;
	fragColor.rgb = mix(fragColor.rgb, teamColor.rgb, fragColor.a); // teamcolor
	fragColor.rgb = fragColor.rgb * reflection + specular;
	#endif

	#if (DEFERRED_MODE == 0 && MAX_DYNAMIC_MODEL_LIGHTS > 0)
	fragColor.rgb += DynamicLighting(normal, diffuse.rgb, specular);
	#endif

	#if (DEFERRED_MODE == 1)
	fragData[GBUFFER_NORMTEX_IDX] = vec4((normal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
	fragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(                      diffuse.rgb, teamColor.rgb,   diffuse.a), alpha);
	fragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(fragData[GBUFFER_DIFFTEX_IDX].rgb, nanoColor.rgb, nanoColor.a), alpha);
	// do not premultiply reflection, leave it to the deferred lighting pass
	// fragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(diffuse.rgb, teamColor.rgb, diffuse.a) * reflection, alpha);
	// allows standard-lighting reconstruction by lazy LuaMaterials using us
	fragData[GBUFFER_SPECTEX_IDX] = vec4(extraColor.rgb, alpha);
	fragData[GBUFFER_EMITTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	fragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	#else
	fragColor.rgb = mix(fogColor.rgb, fragColor.rgb, fogFactor); // fog
	fragColor.rgb = mix(fragColor.rgb, nanoColor.rgb, nanoColor.a); // wireframe or polygon color
	fragColor.a   = alpha;
	#endif
}

