uniform sampler2D       diffuseTex;
uniform sampler2D       shadingTex;
uniform sampler2DShadow shadowTex;
uniform samplerCube     reflectTex;
uniform samplerCube     specularTex;

uniform mat4 cameraMat;
uniform vec4 lightDir;                // WS (mapInfo->light.sunDir)
varying vec3 cameraDir;               // WS
varying vec3 vertexNormal;            // ES

varying float fogFactor;

uniform vec4 unitTeamColor;
uniform vec3 unitAmbientColor;
uniform vec3 unitDiffuseColor;
uniform float unitShadowDensity;

void main() {
	vec4 shadowInt = shadow2DProj(shadowTex, gl_TexCoord[1]);
		shadowInt.x = 1.0 - shadowInt.x;
		shadowInt.x *= unitShadowDensity;
		shadowInt.x = 1.0 - shadowInt.x;

	vec3 vertexNormES = normalize(vertexNormal);
	vec3 cameraDirES = normalize(cameraMat * vec4(normalize(cameraDir), 0.0)).xyz;
	vec3 reflectDirES = normalize(reflect(cameraDirES, vertexNormES));

	float cosAngleDiffuse = min(max(dot(vertexNormES, cameraDirES), 0.0), 1.0);

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
	gl_FragColor.rgb = mix(gl_Fog.color.rgb, gl_FragColor.rgb, fogFactor);
	gl_FragColor.a = shadingCol.a;
}
