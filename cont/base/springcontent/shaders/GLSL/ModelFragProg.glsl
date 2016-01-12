// #define use_normalmapping
// #define flip_normalmap

#define textureS3o1 diffuseTex
#define textureS3o2 shadingTex
  uniform sampler2D textureS3o1;
  uniform sampler2D textureS3o2;
  uniform samplerCube specularTex;
  uniform samplerCube reflectTex;

  uniform vec3 sunDir;
  uniform vec3 sunDiffuse;
  uniform vec3 sunAmbient;

#if (USE_SHADOWS == 1)
  uniform sampler2DShadow shadowTex;
  uniform float shadowDensity;
#endif

  // for opaque passes, tc.a contains a distance fading factor and alphaPass=0.0
  // for alpha passes, tc.a contains one of alphaValues.xyzw and alphaPass=1.0
  uniform vec4 teamColor;
  uniform float alphaPass;

  varying vec4 vertexWorldPos;
  varying vec3 cameraDir;
  varying float fogFactor;

#ifdef use_normalmapping
  uniform sampler2D normalMap;
  varying mat3 tbnMatrix;
#else
  varying vec3 normalv;
#endif



float GetShadowCoeff(vec4 shadowCoors)
{
	#if (USE_SHADOWS == 1)
	float coeff = shadow2DProj(shadowTex, shadowCoors).r;

	coeff  = (1.0 - coeff);
	coeff *= shadowDensity;
	return (1.0 - coeff);
	#else
	return 1.0;
	#endif
}

void main(void)
{
#ifdef use_normalmapping
	vec2 tc = gl_TexCoord[0].st;
	#ifdef flip_normalmap
		tc.t = 1.0 - tc.t;
	#endif
	vec3 nvTS  = normalize((texture2D(normalMap, tc).xyz - 0.5) * 2.0);
	vec3 normal = tbnMatrix * nvTS;
#else
	vec3 normal = normalize(normalv);
#endif

	vec3 light = max(dot(normal, sunDir), 0.0) * sunDiffuse + sunAmbient;

	vec4 diffuse     = texture2D(textureS3o1, gl_TexCoord[0].st);
	vec4 extraColor  = texture2D(textureS3o2, gl_TexCoord[0].st);

	vec3 reflectDir = reflect(cameraDir, normal);
	vec3 specular   = textureCube(specularTex, reflectDir).rgb * extraColor.g * 4.0;
	vec3 reflection = textureCube(reflectTex,  reflectDir).rgb;

	float shadow = GetShadowCoeff(gl_TexCoord[1] + vec4(0.0, 0.0, -0.00005, 0.0));

	// no highlights if in shadow; decrease light to ambient level
	specular *= shadow;
	light = mix(sunAmbient, light, shadow);


	reflection  = mix(light, reflection, extraColor.g); // reflection
	reflection += extraColor.rrr; // self-illum

	#if (DEFERRED_MODE == 0)
	gl_FragColor     = diffuse;
	gl_FragColor.rgb = mix(gl_FragColor.rgb, teamColor.rgb, gl_FragColor.a); // teamcolor
	gl_FragColor.rgb = gl_FragColor.rgb * reflection + specular;
	#endif

	#if (DEFERRED_MODE == 0 && MAX_DYNAMIC_MODEL_LIGHTS > 0)
	for (int i = 0; i < MAX_DYNAMIC_MODEL_LIGHTS; i++) {
		vec3 lightVec = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].position.xyz - vertexWorldPos.xyz;
		vec3 halfVec = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].halfVector.xyz;

		float lightRadius = gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].constantAttenuation;
		float lightDistance = length(lightVec);
		float lightScale = (lightDistance > lightRadius)? 0.0: 1.0;
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

		lightScale *= ((vectorDot < cutoffDot)? 0.0: 1.0);

		gl_FragColor.rgb += (lightScale *                                  gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].ambient.rgb);
		gl_FragColor.rgb += (lightScale * lightAttenuation * (diffuse.rgb * gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].diffuse.rgb * lightCosAngDiff));
		gl_FragColor.rgb += (lightScale * lightAttenuation * (specular.rgb * gl_LightSource[BASE_DYNAMIC_MODEL_LIGHT + i].specular.rgb * pow(lightCosAngSpec, 4.0)));
	}
	#endif

	#if (DEFERRED_MODE == 1)
	gl_FragData[GBUFFER_NORMTEX_IDX] = vec4((normal + vec3(1.0, 1.0, 1.0)) * 0.5, 1.0);
	gl_FragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(diffuse.rgb, teamColor.rgb, diffuse.a), extraColor.a * teamColor.a);
	// do not premultiply reflection, leave it to the deferred lighting pass
	// gl_FragData[GBUFFER_DIFFTEX_IDX] = vec4(mix(diffuse.rgb, teamColor.rgb, diffuse.a) * reflection, extraColor.a * teamColor.a);
	// allows standard-lighting reconstruction by lazy LuaMaterials using us
	gl_FragData[GBUFFER_SPECTEX_IDX] = extraColor;
	gl_FragData[GBUFFER_EMITTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	gl_FragData[GBUFFER_MISCTEX_IDX] = vec4(0.0, 0.0, 0.0, 0.0);
	#else
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor); // fog
	gl_FragColor.a   = mix(extraColor.a * teamColor.a, teamColor.a, alphaPass);
	#endif
}

