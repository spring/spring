#define textureS3o1 diffuseTex
#define textureS3o2 shadingTex
	uniform sampler2D textureS3o1;
	uniform sampler2D textureS3o2;
	uniform samplerCube specularTex;
	uniform samplerCube reflectTex;

	uniform vec3 sunDir;
	uniform vec3 sunDiffuse;
	uniform vec3 sunAmbient;
	uniform vec3 sunSpecular;
#if (USE_SHADOWS == 1)
	varying vec4 shadowVertexPos;
	uniform sampler2DShadow shadowTex;
	uniform sampler2D shadowColorTex;
	uniform float shadowDensity;
#endif

	// in opaque passes tc.a is always 1.0 [all objects], and alphaPass is 0.0
	// in alpha passes tc.a is either one of alphaValues.xyzw [for units] *or*
	// contains a distance fading factor [for features], and alphaPass is 1.0
	// texture alpha-masking is done in both passes
	uniform vec4 teamColor;
	uniform vec4 nanoColor;

	varying vec4 vertexWorldPos;
	varying vec3 cameraDir;
	varying float fogFactor;
#ifdef use_normalmapping
	uniform sampler2D normalMap;
	varying mat3 tbnMatrix;
#else
	varying vec3 normalv;
#endif


vec3 GetShadowMult(float NdotL) {
	#if (USE_SHADOWS == 1)
		vec3 shadowCoord = shadowVertexPos.xyz / shadowVertexPos.w;
		float sh = min(shadow2DProj(shadowTex, shadowVertexPos).r, smoothstep(0.0, 0.35, NdotL));
		vec3 shColor = texture2D(shadowColorTex, shadowCoord.xy).rgb;
		return mix(1.0, sh, shadowDensity) * shColor;
	#else
		return vec3(1.0);
	#endif
}

vec3 DynamicLighting(vec3 normal, vec3 diffuse, vec3 specular) {
	vec3 rgb = vec3(0.0);

	for (int i = 0; i < MAX_DYNAMIC_MODEL_LIGHTS; i++) {
		vec3 lightVec = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].position.xyz - vertexWorldPos.xyz;
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

	return rgb;
}

void main(void)
{
	vec3 normal = normalize(normalv);

	float NdotLu = dot(normal, sunDir);
	float NdotL = max(NdotLu, 1e-3);
	vec3 light = NdotL * sunDiffuse + sunAmbient;

	vec4 diffuse     = texture2D(textureS3o1, gl_TexCoord[0].st);
	vec4 extraColor  = texture2D(textureS3o2, gl_TexCoord[0].st);

	vec3 reflectDir = reflect(cameraDir, normal);
	vec3 specular   = textureCube(specularTex, reflectDir).rgb * sunSpecular;
	vec3 reflection = textureCube(reflectTex,  reflectDir).rgb;

	vec3 shadowMult = GetShadowMult(NdotL);
	float alpha = teamColor.a * extraColor.a; // apply one-bit mask

	specular *= (extraColor.g * 4.0);
	// no highlights if in shadowMult; decrease light to ambient level
	specular *= shadowMult;
	light = mix(sunAmbient, light, shadowMult);


	reflection  = mix(light, reflection, extraColor.g); // reflection
	reflection += extraColor.rrr; // self-illum

#if (DEFERRED_MODE == 0)
	gl_FragColor     = diffuse;
	gl_FragColor.rgb = mix(gl_FragColor.rgb, teamColor.rgb, gl_FragColor.a); // teamcolor
	gl_FragColor.rgb = gl_FragColor.rgb * reflection + specular;
#endif

#if (DEFERRED_MODE == 0 && MAX_DYNAMIC_MODEL_LIGHTS > 0)
	gl_FragColor.rgb += DynamicLighting(normal, diffuse.rgb, specular);
#endif

#if (DEFERRED_MODE == 1)
	gl_FragData[GBUFFER_NORMTEX_IDX] = vec4((normal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
	gl_FragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(                         diffuse.rgb, teamColor.rgb,   diffuse.a), alpha);
	gl_FragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(gl_FragData[GBUFFER_DIFFTEX_IDX].rgb, nanoColor.rgb, nanoColor.a), alpha);
	// do not premultiply reflection, leave it to the deferred lighting pass
	// gl_FragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(diffuse.rgb, teamColor.rgb, diffuse.a) * reflection, alpha);
	// allows standard-lighting reconstruction by lazy LuaMaterials using us
	gl_FragData[GBUFFER_SPECTEX_IDX] = vec4(extraColor.rgb, alpha);
	gl_FragData[GBUFFER_EMITTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	gl_FragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
#else
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor); // fog
	gl_FragColor.rgb = mix(gl_FragColor.rgb, nanoColor.rgb, nanoColor.a); // wireframe or polygon color
	gl_FragColor.a   = alpha;
#endif
}

