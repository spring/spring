#version 410 core

uniform sampler2D       diffuseTex;
uniform sampler2DShadow shadowTex;

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;

uniform float groundShadowDensity;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

uniform vec4 fogColor;


in vec4 vVertexPos;
in vec2 vTexCoord;
in vec4 vFrontColor;

in float vFogFactor;

layout(location = 0) out vec4 fFragColor;


void main() {
	#if (TREE_SHADOW == 1)
	// per-fragment
	vec4 vertexShadowPos = shadowMatrix * vVertexPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	float shadowCoeff = mix(1.0, textureProj(shadowTex, vertexShadowPos), groundShadowDensity);
	#else
	float shadowCoeff = 1.0;
	#endif

	vec4 diffuseCol = texture(diffuseTex, vTexCoord) * vFrontColor;
	vec3 shadeInt = mix(groundAmbientColor.rgb, vec3(1.0, 1.0, 1.0), shadowCoeff);


	fFragColor.rgb = diffuseCol.rgb * shadeInt.rgb;
	fFragColor.rgb = mix(fogColor.rgb, fFragColor.rgb, vFogFactor);

	fFragColor.a = diffuseCol.a;
}
