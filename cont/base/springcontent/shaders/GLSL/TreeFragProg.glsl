#version 410 core

uniform sampler2D       diffuseTex;
uniform sampler2DShadow shadowTex;

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;

uniform float groundShadowDensity;
uniform float gammaExponent;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

uniform vec4 alphaTestCtrl;

uniform vec4 fogColor;


in vec4 vVertexPos;
in vec2 vTexCoord;
in vec4 vBaseColor;

in float vFogFactor;

layout(location = 0) out vec4 fFragColor;


void main() {
	#if (TREE_SHADOW == 1)
	// per-fragment
	vec4 vertexShadowPos = shadowMatrix * vVertexPos;

	float shadowCoeff = mix(1.0, textureProj(shadowTex, vertexShadowPos), groundShadowDensity);
	#else
	float shadowCoeff = 1.0;
	#endif

	vec4 diffuseCol = texture(diffuseTex, vTexCoord) * vBaseColor;
	vec3 shadeInt = mix(groundAmbientColor.rgb, vec3(1.0, 1.0, 1.0), shadowCoeff);

	{
		float alphaTestGreater = float(diffuseCol.a > alphaTestCtrl.x) * alphaTestCtrl.y;
		float alphaTestSmaller = float(diffuseCol.a < alphaTestCtrl.x) * alphaTestCtrl.z;

		if ((alphaTestGreater + alphaTestSmaller + alphaTestCtrl.w) == 0.0)
			discard;
	}

	fFragColor.rgb = diffuseCol.rgb * shadeInt.rgb;
	fFragColor.rgb = mix(fogColor.rgb, fFragColor.rgb, vFogFactor);
	fFragColor.rgb = pow(fFragColor.rgb, vec3(gammaExponent));

	fFragColor.a = diffuseCol.a;
}
