// ARB shader receives groundAmbientColor multiplied
// by this constant; shading-texture intensities are
// also pre-dimmed
#define SMF_INTENSITY_MUL (210.0 / 255.0)
#define SSMF_UNCOMPRESSED_NORMALS 0

uniform vec4 lightDir;

uniform sampler2D       diffuseTex;
uniform sampler2D       normalsTex;
uniform sampler2DShadow shadowTex;
uniform sampler2D       detailTex;
uniform sampler2D       specularTex;

#if (HAVE_SHADOWS == 1)
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

	// per-channel splat intensity multipliers
	uniform vec4 splatTexMults;
#endif

#if (SMF_SKY_REFLECTIONS == 1)
	uniform samplerCube skyReflectTex;
	uniform sampler2D skyReflectModTex;
#endif

#if (SMF_DETAIL_NORMALS == 1)
	uniform sampler2D detailNormalTex;
#endif

uniform vec3 cameraPos;
varying vec3 halfDir;

varying float fogFactor;

varying vec4 vertexWorldPos;
varying vec2 diffuseTexCoords;
varying vec2 specularTexCoords;
varying vec2 normalTexCoords;

uniform int numMapDynLights;


void main() {
	// we don't calculate it in the vertex shader to save varyings (OpenGL2.0 just allows 32 varying components)
	vec3 cameraDir = vertexWorldPos.xyz - cameraPos;

	#if (HAVE_SHADOWS == 1)
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = shadowMat * vertexWorldPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;
	#endif

	vec3 normal;
	#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		normal = normalize((texture2D(normalsTex, normalTexCoords).rgb * 2.0) - 1.0);
	#else
		normal.xz = (texture2D(normalsTex, normalTexCoords).ra * 2.0) - 1.0;
		normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	#endif

	#if (SMF_DETAIL_NORMALS == 1)
	// detail-normals are (assumed to be) defined within STN space
	// (for a regular vertex normal equal to <0, 1, 0>, the S- and
	// T-tangents are aligned with Spring's +x and +z (!) axes)
	vec3 tTangent = cross(normal, vec3(-1.0, 0.0, 0.0));
	vec3 sTangent = cross(normal, tTangent);
	vec4 dtSample = texture2D(detailNormalTex, normalTexCoords);
	vec3 dtNormal = (dtSample.rgb * 2.0) - 1.0;
	mat3 stnMatrix = mat3(sTangent, tTangent, normal);

	normal = normalize((normal * (1.0 - dtSample.a)) + ((stnMatrix * dtNormal) * dtSample.a));
	#endif


	float cosAngleDiffuse = clamp(dot(normalize(lightDir.xyz), normal), 0.0, 1.0);
	float cosAngleSpecular = clamp(dot(normalize(halfDir), normal), 0.0, 1.0);
	vec4 diffuseCol = texture2D(diffuseTex, diffuseTexCoords);
	vec4 specularCol = texture2D(specularTex, specularTexCoords);

	#if (SMF_ARB_LIGHTING == 0)
	// sun specular lighting contribution
	float specularExp = specularCol.a * 16.0;
	float specularPow = pow(cosAngleSpecular, specularExp);
	vec3 specularInt  = specularCol.rgb * specularPow;
	#endif


	vec4 detailCol;
	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
		detailCol = (texture2D(detailTex, gl_TexCoord[0].st) * 2.0) - 1.0;
	#else
		vec4 splatDetails;
			splatDetails.r = texture2D(splatDetailTex, gl_TexCoord[0].st).r;
			splatDetails.g = texture2D(splatDetailTex, gl_TexCoord[0].pq).g;
			splatDetails.b = texture2D(splatDetailTex, gl_TexCoord[1].st).b;
			splatDetails.a = texture2D(splatDetailTex, gl_TexCoord[1].pq).a;
			splatDetails   = (splatDetails * 2.0) - 1.0;
		vec4 splatCofac = texture2D(splatDistrTex, specularTexCoords) * splatTexMults;

		detailCol.rgb = vec3(dot(splatDetails, splatCofac));
		detailCol.a = 1.0;
	#endif


	#if (SMF_SKY_REFLECTIONS == 1)
		vec3 reflectDir = reflect(cameraDir, normal); //cameraDir doesn't need to be normalized for reflect()
		vec3 reflectCol = textureCube(skyReflectTex, gl_NormalMatrix * reflectDir).rgb;
		vec3 reflectMod = texture2D(skyReflectModTex, specularTexCoords).rgb;

		diffuseCol.rgb = mix(diffuseCol.rgb, reflectCol, reflectMod);
	#endif



	#if (HAVE_SHADOWS == 1)
	float shadowInt = shadow2DProj(shadowTex, vertexShadowPos).r;
	#else
	float shadowInt = 1.0;
	#endif

	// NOTE: do we want to add the ambient term to diffuseInt?
	// vec4 diffuseInt = texture2D(shadingTex, diffuseTexCoords);
	vec3 diffuseInt = groundAmbientColor + groundDiffuseColor * cosAngleDiffuse;
	vec3 ambientInt = groundAmbientColor;
	vec4 shadeInt;

	#if (SMF_ARB_LIGHTING == 0)
	specularInt *= shadowInt;
	#endif

	#if (HAVE_SHADOWS == 1)
	shadowInt = 1.0 - (1.0 - shadowInt) * groundShadowDensity;
	shadeInt.rgb = mix(ambientInt, diffuseInt, shadowInt);
	#else
	shadeInt.rgb = diffuseInt;
	#endif

	shadeInt.rgb *= SMF_INTENSITY_MUL;
	shadeInt.a = 1.0;


	#if (SMF_WATER_ABSORPTION == 1)
		if (vertexWorldPos.y < 0.0) {
			float vertexStepHeight = min(1023.0, -floor(vertexWorldPos.y));
			float waterLightInt = min((cosAngleDiffuse + 0.2) * 2.0, 1.0);
			vec4 waterHeightColor;
				waterHeightColor.rgb = max(waterMinColor.rgb, waterBaseColor.rgb - (waterAbsorbColor.rgb * vertexStepHeight)) * SMF_INTENSITY_MUL;
				waterHeightColor.a = max(0.0, (255.0 + 10.0 * vertexWorldPos.y) / 255.0);

			if (vertexWorldPos.y > -10.0) {
				shadeInt.rgb =
					(diffuseInt.rgb * (1.0 - (-vertexWorldPos.y * 0.1))) +
					(waterHeightColor.rgb * (0.0 + (-vertexWorldPos.y * 0.1)) * waterLightInt);
			} else {
				shadeInt.rgb = waterHeightColor.rgb * waterLightInt;
			}

			shadeInt.a = waterHeightColor.a;
			shadeInt *= shadowInt;
		}
	#endif


	gl_FragColor = (diffuseCol + detailCol) * shadeInt;

	#if (SMF_ARB_LIGHTING == 0)
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
		float lightSpecularPow = pow(lightCosAngSpec, specularExp);
		#else
		float lightSpecularPow = 0.0;
		#endif

		lightScale *= ((vectorDot < cutoffDot)? 0.0: 1.0);

		gl_FragColor.rgb += (lightScale *                                     gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].ambient.rgb);
		gl_FragColor.rgb += (lightScale * lightAttenuation * (diffuseCol.rgb * gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].diffuse.rgb * lightCosAngDiff));
		gl_FragColor.rgb += (lightScale * lightAttenuation * (specularCol.rgb * gl_LightSource[BASE_DYNAMIC_MAP_LIGHT + i].specular.rgb * lightSpecularPow));
	}
	#endif

	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fogFactor);
	gl_FragColor.a = min(diffuseCol.a, (vertexWorldPos.y * 0.1) + 1.0);
}
