#version 410 core

uniform mat4 projMat;
uniform mat4 viewMat;
uniform mat4 treeMat;             // world-transform

uniform vec3 cameraDirX;          // needed for bush-type trees
uniform vec3 cameraDirY;

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;

uniform vec2 alphaModifiers;      // (tree-height alpha, ground-diffuse alpha)

uniform vec3 fogParams;           // .x := start, .y := end, .z := viewrange


layout(location = 0) in vec3 vtxPositionAttr;
layout(location = 1) in vec2 vtxTexCoordAttr;
layout(location = 2) in vec3 vtxNormalAttr;

out vec4 vVertexPos;
out vec2 vTexCoord;
out vec4 vBaseColor;

out float vFogFactor;


void main() {
	vec4 vertexPos = vec4(vtxPositionAttr, 1.0);

	vertexPos.xyz += (cameraDirX * vtxNormalAttr.x);
	vertexPos.xyz += (cameraDirY * vtxNormalAttr.y);

	#if (TREE_SHADOW == 1)
	vVertexPos = treeMat * vertexPos;
	#endif

	vBaseColor.rgb = (vtxNormalAttr.z * groundDiffuseColor.rgb) + groundAmbientColor.rgb;
	vBaseColor.a = (vtxPositionAttr.y * alphaModifiers.x) + alphaModifiers.y;

	vTexCoord = vtxTexCoordAttr;


	gl_Position = projMat * viewMat * treeMat * vertexPos;

	{
		float eyeDepth = length((viewMat * treeMat * vertexPos).xyz);
		float fogRange = (fogParams.y - fogParams.x) * fogParams.z;
		float fogDepth = (eyeDepth - fogParams.x * fogParams.z) / fogRange;
		// float fogDepth = (fogParams.y * fogParams.z - eyeDepth) / fogRange;

		vFogFactor = 1.0 - clamp(fogDepth, 0.0, 1.0);
		// vFogFactor = clamp(fogDepth, 0.0, 1.0);
	}
}

