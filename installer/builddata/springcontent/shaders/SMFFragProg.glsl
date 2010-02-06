uniform vec4 lightDir;
varying vec3 viewDir;
varying vec3 halfDir;

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

// ARB shader receives dimmed groundAmbientColor
#define GROUND_AMBIENT_COLOR_MUL (210 / 255.0)

#define SMF_ARB_LIGHTING 0

void main() {
	vec2 tc0 = gl_TexCoord[0].st;
	vec2 tc1 = gl_TexCoord[1].st;
	vec2 tc3 = gl_TexCoord[3].st;

	vec4 vertexShadowPos = shadowMat * gl_TexCoord[2];
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + shadowParams.z) + shadowParams.w);
		vertexShadowPos.st += shadowParams.xy;

	vec3 normal = normalize((texture2D(normalsTex, tc1) * 2.0).rgb - 1.0);

	float cosAngleDiffuse = min(max(dot(normalize(lightDir.xyz), normal), 0.0), 1.0);
	float cosAngleSpecular = min(max(dot(normalize(halfDir), normal), 0.0), 1.0);
	float specularExp = texture2D(specularTex, tc1).a * 16.0;
	float specularPow = pow(cosAngleSpecular, specularExp);


	vec4 diffuseCol = texture2D(diffuseTex, tc0);
	vec3 specularCol = texture2D(specularTex, tc1).rgb;
	vec4 detailCol = normalize((texture2D(detailTex, tc3) * 2.0) - 1.0);

	// vec4 diffuseInt = texture2D(shadingTex, tc0);
	vec4 diffuseInt =
		vec4(groundAmbientColor, 1.0) +
		vec4(groundDiffuseColor, 1.0) * cosAngleDiffuse;
	vec4 ambientInt = vec4(groundAmbientColor, 1.0) * GROUND_AMBIENT_COLOR_MUL;
	vec3 specularInt = specularCol * specularPow;
	vec4 shadowInt = shadow2DProj(shadowTex, vertexShadowPos);
		shadowInt.x = 1.0 - shadowInt.x;
		shadowInt.x *= groundShadowDensity;
		// shadowInt.x *= diffuseInt.w;
		shadowInt.x = 1.0 - shadowInt.x;

	// vec4 shadeCol = (shadowInt.x * diffuseInt) + ((1.0 - shadowInt.x) * ambientInt);
	vec4 shadeCol = mix(ambientInt, diffuseInt, shadowInt.x);

	gl_FragColor = (diffuseCol + detailCol) * shadeCol;
	gl_FragColor.a = (gl_TexCoord[0].q * 0.1) + 1.0;

	#if (SMF_ARB_LIGHTING == 0)
	// no need to multiply by groundSpecularColor anymore
	gl_FragColor.rgb += specularInt;
	#endif
}
