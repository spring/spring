// ARB shader receives groundAmbientColor multiplied
// by this constant; shading-texture intensities are
// also pre-dimmed
#define SMF_INTENSITY_MUL (210.0 / 255.0)
#define SMF_ARB_LIGHTING 0
#define SSMF_UNCOMPRESSED_NORMALS 0

uniform vec4 lightDir;

uniform sampler2D       diffuseTex;
uniform sampler2D       normalsTex;
uniform sampler2DShadow shadowTex;
uniform sampler2D       detailTex;
uniform sampler2D       specularTex;

uniform mat4 shadowMat;
uniform vec4 shadowParams;

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

uniform vec3 cameraPos;
varying vec3 halfDir;

varying float fogFactor;

varying vec4 vertexPos;
varying vec2 diffuseTexCoords;
varying vec2 specularTexCoords;
varying vec2 normalTexCoords;


void main() {
	// we don't calculate it in the vertex shader to save varyings (OpenGL2.0 just allows 32 varying components)
	vec3 cameraDir = vertexPos.xyz - cameraPos;

	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = shadowMat * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	vec3 normal;
	#if (SSMF_UNCOMPRESSED_NORMALS == 1)
		normal = normalize((texture2D(normalsTex, normalTexCoords).rgb * 2.0) - 1.0);
	#else
		normal.xz = (texture2D(normalsTex, normalTexCoords).ra * 2.0) - 1.0;
		normal.y  = sqrt(1.0 - dot(normal.xz, normal.xz));
	#endif

	float cosAngleDiffuse = clamp(dot(normalize(lightDir.xyz), normal), 0.0, 1.0);
	vec4 diffuseCol = texture2D(diffuseTex, diffuseTexCoords);

	float cosAngleSpecular = clamp(dot(normalize(halfDir), normal), 0.0, 1.0);
	vec4 specularFrag = texture2D(specularTex, specularTexCoords);
	float specularExp = specularFrag.a * 16.0;
	float specularPow = pow(cosAngleSpecular, specularExp);
	vec3 specularInt  = specularFrag.rgb * specularPow;

	vec4 detailCol;
	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
		detailCol = (texture2D(detailTex, gl_TexCoord[0].st) * 2.0) - 1.0;
	#else
		vec4 splatDetails;
		splatDetails.r = texture2D(splatDetailTex, gl_TexCoord[0].st).r;
		splatDetails.g = texture2D(splatDetailTex, gl_TexCoord[0].pq).g;
		splatDetails.b = texture2D(splatDetailTex, gl_TexCoord[1].st).b;
		splatDetails.a = texture2D(splatDetailTex, gl_TexCoord[1].pq).a;
		vec4 splatCofac = texture2D(splatDistrTex, specularTexCoords) * splatTexMults;
		splatDetails = (splatDetails * 2.0) - 1.0;
		detailCol.rgb = vec3(dot(splatDetails, splatCofac));
		detailCol.a = 1.0;
	#endif


	#if (SMF_SKY_REFLECTIONS == 1)
		vec3 reflectDir = reflect(cameraDir, normal); //cameraDir doesn't need to be normalized for reflect()
		vec3 reflectCol = textureCube(skyReflectTex, gl_NormalMatrix * reflectDir).rgb;
		vec3 reflectMod = texture2D(skyReflectModTex, specularTexCoords).rgb;

		diffuseCol.rgb = mix(diffuseCol.rgb, reflectCol, reflectMod);
	#endif


	// vec4 diffuseInt = texture2D(shadingTex, diffuseTexCoords);
	float shadowInt = shadow2DProj(shadowTex, vertexShadowPos).r;
	specularInt *= shadowInt;
	shadowInt = 1.0 - (1.0 - shadowInt) * groundShadowDensity;
	vec3 diffuseInt = groundAmbientColor + groundDiffuseColor * cosAngleDiffuse;

	vec3 ambientInt = groundAmbientColor;
	vec4 shadeInt;
	shadeInt.rgb = mix(ambientInt, diffuseInt, shadowInt);
	shadeInt.rgb *= SMF_INTENSITY_MUL;
	shadeInt.a = 1.0;


	#if (SMF_WATER_ABSORPTION == 1)
		if (vertexPos.y < 0.0) {
			float vertexStepHeight = min(1023.0, -floor(vertexPos.y));
			float waterLightInt = min((cosAngleDiffuse + 0.2) * 2.0, 1.0);
			vec4 waterHeightColor;
			waterHeightColor.rgb = max(waterMinColor.rgb, waterBaseColor.rgb - (waterAbsorbColor.rgb * vertexStepHeight)) * SMF_INTENSITY_MUL;
			waterHeightColor.a = max(0.0, (255.0 + 10.0 * vertexPos.y) / 255.0);

			if (vertexPos.y > -10.0) {
				shadeInt.rgb =
					(diffuseInt.rgb * (1.0 - (-vertexPos.y * 0.1))) +
					(waterHeightColor.rgb * (0.0 + (-vertexPos.y * 0.1)) * waterLightInt);
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

	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fogFactor);
	gl_FragColor.a = min(diffuseCol.a, (vertexPos.y * 0.1) + 1.0);
}
