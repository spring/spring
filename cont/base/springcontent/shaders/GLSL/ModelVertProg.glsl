#version 410 core

layout(location = 0) in vec3 positionAttr;
layout(location = 1) in vec3   normalAttr;
layout(location = 2) in vec3 sTangentAttr;
layout(location = 3) in vec3 tTangentAttr;
layout(location = 4) in vec2 texCoor0Attr;
layout(location = 5) in vec2 texCoor1Attr;
layout(location = 6) in uint pieceIdxAttr;


// #define use_normalmapping
// #define flip_normalmap

uniform mat4 pieceMatrices[128];

uniform mat4 modelMatrix;
uniform mat4  viewMatrix;
uniform mat4  projMatrix;

uniform vec3 cameraPos;
uniform vec3 fogParams;

uniform vec4 waterClipPlane;
uniform vec4 upperClipPlane;
uniform vec4 lowerClipPlane;


out vec4 worldPos;
out vec3 cameraDir;

out vec2 texCoord0;
// out vec2 texCoord1;

out float gl_ClipDistance[MDL_CLIP_PLANE_IDX + 1];
out float fogFactor;

#ifdef use_normalmapping
out mat3 tbnMatrix;
#else
out vec3 wsNormalVec;
#endif


mat4 mat4mix(mat4 a, mat4 b, float alpha) {
	return (a * (1.0 - alpha) + b * alpha);
}

void main(void)
{
	// mat4 pieceMatrix = mat4mix(mat4(1.0), pieceMatrices[pieceIdxAttr], pieceMatrices[0][3][3]);
	mat4 pieceMatrix = pieceMatrices[pieceIdxAttr];
	mat4 modelPieceMatrix = modelMatrix * pieceMatrix;

	vec4 vertexPos = vec4(positionAttr, 1.0);
	vec4 vertexModelPos = modelPieceMatrix * vertexPos;
	vec4 vertexViewPos = viewMatrix * vertexModelPos;

#ifdef use_normalmapping
	tbnMatrix = modelPieceMatrix * mat3(sTangentAttr, tTangentAttr, normalAttr);
#else
	// NOTE:
	//   technically need inverse(transpose(mview)) here, but
	//   mview just equals model here and model is orthonormal
	wsNormalVec = (modelPieceMatrix * vec4(normalAttr, 0.0)).xyz;
#endif

	gl_Position = projMatrix * vertexViewPos;

	worldPos  = vertexModelPos;
	cameraDir = vertexModelPos.xyz - cameraPos;

	texCoord0 = texCoor0Attr;
	// texCoord1 = texCoor1Attr;

	// clip reflections in model-space
	gl_ClipDistance[MDL_CLIP_PLANE_IDX - 2] = dot(upperClipPlane, vertexPos);
	gl_ClipDistance[MDL_CLIP_PLANE_IDX - 1] = dot(lowerClipPlane, vertexPos);
	gl_ClipDistance[MDL_CLIP_PLANE_IDX - 0] = dot(waterClipPlane, vertexModelPos);


	#if (DEFERRED_MODE == 0)
	{
		float eyeDepth = length(vertexViewPos.xyz);
		float fogRange = (fogParams.y - fogParams.x) * fogParams.z;
		float fogDepth = (eyeDepth - fogParams.x * fogParams.z) / fogRange;
		// float fogDepth = (fogParams.y * fogParams.z - eyeDepth) / fogRange;

		fogFactor = 1.0 - clamp(fogDepth, 0.0, 1.0);
		// fogFactor = clamp(fogDepth, 0.0, 1.0);
	}
	#endif
}

