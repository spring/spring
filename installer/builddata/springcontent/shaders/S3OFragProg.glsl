uniform sampler2D       diffuseTex;
uniform sampler2D       shadingTex;
uniform sampler2DShadow shadowTex;
uniform samplerCube     reflectTex;
uniform samplerCube     specularTex;

uniform mat4 shadowMat;
uniform vec4 shadowParams;

uniform vec4 lightDir;                // mapInfo->light.sunDir
varying vec3 cameraDir;
varying vec3 vertexNormal;

uniform vec4 unitTeamColor;
uniform vec3 unitAmbientColor;
uniform vec3 unitDiffuseColor;
uniform float unitShadowDensity;

void main() {
	vec4 vertexShadowPos = shadowMat * gl_TexCoord[1];
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + shadowParams.z) + shadowParams.w);
		vertexShadowPos.st += shadowParams.xy;

	vec4 shadowInt = shadow2DProj(shadowTex, vertexShadowPos);
		shadowInt.x = 1.0 - shadowInt.x;
		shadowInt.x *= unitShadowDensity;
		shadowInt.x = 1.0 - shadowInt.x;

	vec3 vertexNormES = gl_NormalMatrix * normalize(vertexNormal);
	vec3 cameraDirES = gl_NormalMatrix * normalize(cameraDir);
	vec3 reflectDirES = normalize(reflect(cameraDirES, vertexNormES));

	float cosAngleDiffuse = min(max(dot(vertexNormES, gl_NormalMatrix * lightDir.xyz), 0.0), 1.0);

	vec4 diffuseCol = texture2D(diffuseTex, gl_TexCoord[0].st);
	vec4 shadingCol = texture2D(shadingTex, gl_TexCoord[0].st);
	vec3 specularCol = textureCube(specularTex, reflectDirES).rgb * (shadingCol.g * 4.0 * shadowInt.x);
	vec3 reflectCol = textureCube(reflectTex, reflectDirES).rgb;

	vec3 diffuseInt = unitDiffuseColor * cosAngleDiffuse + unitAmbientColor;

	// green channel of shadingTex moderates both specularity and reflection
	diffuseInt = mix(unitAmbientColor, diffuseInt, shadowInt.x);
	diffuseInt = mix(diffuseInt, reflectCol, shadingCol.g);
	diffuseInt += shadingCol.r;
	diffuseCol = mix(diffuseCol, unitTeamColor, diffuseCol.a);

	gl_FragColor.rgb = diffuseCol.rgb * diffuseInt + specularCol.rgb;
	gl_FragColor.a = shadingCol.a;
}
