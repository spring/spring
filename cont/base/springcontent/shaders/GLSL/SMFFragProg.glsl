// ARB shader receives groundAmbientColor multiplied
// by this constant; shading-texture intensities are
// also pre-dimmed
#define SMF_INTENSITY_MUL (210 / 255.0)
#define SMF_ARB_LIGHTING 0

uniform vec4 lightDir;
varying vec3 cameraDir;
varying vec3 viewDir;
varying vec3 halfDir;

varying float fogFactor;
varying float vertexHeight;

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


void main() {
	vec2 tc0 = gl_TexCoord[0].st;
	vec2 tc1 = gl_TexCoord[1].st;
	vec2 tc2 = gl_TexCoord[2].st;

	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = shadowMat * gl_TexCoord[3];
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	vec3 normal = normalize((texture2D(normalsTex, tc1) * 2.0).rgb - 1.0);

	float cosAngleDiffuse = min(max(dot(normalize(lightDir.xyz), normal), 0.0), 1.0);
	float cosAngleSpecular = min(max(dot(normalize(halfDir), normal), 0.0), 1.0);
	float specularExp = texture2D(specularTex, tc2).a * 16.0;
	float specularPow = pow(cosAngleSpecular, specularExp);


	vec4 diffuseCol = texture2D(diffuseTex, tc0);
	vec3 specularCol = texture2D(specularTex, tc2).rgb;

	#if (SMF_DETAIL_TEXTURE_SPLATTING == 0)
	vec4 detailCol = normalize((texture2D(detailTex, gl_TexCoord[4].st) * 2.0) - 1.0);
	#else
	vec4 splatDistr = texture2D(splatDistrTex, tc2);
	float detailInt =
		(((texture2D(splatDetailTex, gl_TexCoord[4].st) * 2.0).r - 1.0) * (splatTexMults.r * splatDistr.r)) +
		(((texture2D(splatDetailTex, gl_TexCoord[5].st) * 2.0).g - 1.0) * (splatTexMults.g * splatDistr.g)) +
		(((texture2D(splatDetailTex, gl_TexCoord[6].st) * 2.0).b - 1.0) * (splatTexMults.b * splatDistr.b)) +
		(((texture2D(splatDetailTex, gl_TexCoord[7].st) * 2.0).a - 1.0) * (splatTexMults.a * splatDistr.a));
	vec4 detailCol = vec4(detailInt, detailInt, detailInt, 1.0);
	#endif


	#if (SMF_SKY_REFLECTIONS == 1)
	vec3 reflectDir = reflect(normalize(cameraDir), normal);
	vec3 reflectCol = textureCube(skyReflectTex, normalize(gl_NormalMatrix * reflectDir)).rgb;
	vec3 reflectMod = texture2D(skyReflectModTex, tc2);

	diffuseCol.r = mix(diffuseCol.r, reflectCol.r, reflectMod.r);
	diffuseCol.g = mix(diffuseCol.g, reflectCol.g, reflectMod.g);
	diffuseCol.b = mix(diffuseCol.b, reflectCol.b, reflectMod.b);
	#endif


	// vec4 diffuseInt = texture2D(shadingTex, tc0);
	vec4 diffuseInt =
		vec4(groundAmbientColor, 1.0) +
		vec4(groundDiffuseColor, 1.0) * cosAngleDiffuse;
	vec4 ambientInt = vec4(groundAmbientColor, 1.0);
	vec3 specularInt = specularCol * specularPow;
	vec4 shadowInt = shadow2DProj(shadowTex, vertexShadowPos);
		shadowInt.x = 1.0 - shadowInt.x;
		shadowInt.x *= groundShadowDensity;
	//	shadowInt.x *= diffuseInt.w;
		shadowInt.x = 1.0 - shadowInt.x;
	vec4 shadeInt;

	ambientInt.xyz *= SMF_INTENSITY_MUL;
	diffuseInt.xyz *= SMF_INTENSITY_MUL;
	diffuseInt.a = 1.0;
	shadeInt = mix(ambientInt, diffuseInt, shadowInt.x);


	#if (SMF_WATER_ABSORPTION == 1)
		if (vertexHeight < 0.0) {
			int vertexStepHeight = min(1023, int(-vertexHeight));
			float waterLightInt = min((cosAngleDiffuse + 0.2) * 2.0, 1.0);
			vec4 waterHeightColor = vec4(
				max(waterMinColor.r, waterBaseColor.r - (waterAbsorbColor.r * vertexStepHeight)) * SMF_INTENSITY_MUL,
				max(waterMinColor.g, waterBaseColor.g - (waterAbsorbColor.g * vertexStepHeight)) * SMF_INTENSITY_MUL,
				max(waterMinColor.b, waterBaseColor.b - (waterAbsorbColor.b * vertexStepHeight)) * SMF_INTENSITY_MUL,
				max(0.0, (255.0 + int(10.0 * vertexHeight)) / 255.0)
			);

			if (vertexHeight > -10.0) {
				shadeInt.xyz =
					(diffuseInt.xyz * (1.0 - (-vertexHeight * 0.1))) +
					(waterHeightColor.xyz * (0.0 + (-vertexHeight * 0.1)) * waterLightInt);
			} else {
				shadeInt.xyz = (waterHeightColor.xyz * waterLightInt);
			}

			shadeInt.a = waterHeightColor.a;
			shadeInt *= shadowInt.x;
		}
	#endif


	gl_FragColor = (diffuseCol + detailCol) * shadeInt;
	gl_FragColor.a = (gl_TexCoord[0].q * 0.1) + 1.0;

	#if (SMF_ARB_LIGHTING == 0)
		// no need to multiply by groundSpecularColor anymore
		gl_FragColor.rgb += specularInt;
	#endif

	gl_FragColor = mix(gl_Fog.color, gl_FragColor, fogFactor);
}
