uniform mat4 shadowViewMat;
uniform mat4 shadowProjMat;
uniform mat4 modelMat;
uniform mat4 pieceMats[128];
uniform vec4 shadowParams;
uniform vec4 upperClipPlane;
uniform vec4 lowerClipPlane;


layout(location = 0) in vec3 positionAttr;
// layout(location = 1) in vec3   normalAttr;
// layout(location = 2) in vec3 sTangentAttr;
// layout(location = 3) in vec3 tTangentAttr;
layout(location = 4) in vec2 texCoor0Attr;
// layout(location = 5) in vec2 texCoor1Attr;
layout(location = 6) in uint pieceIdxAttr;

out float gl_ClipDistance[2];
out vec2 vTexCoord;


mat4 mat4mix(mat4 a, mat4 b, float alpha) {
	return (a * (1.0 - alpha) + b * alpha);
}

void main() {
	// mat4 pieceMat = mat4mix(mat4(1.0), pieceMats[pieceIdxAttr], pieceMats[0][3][3]);
	mat4 pieceMat = pieceMats[pieceIdxAttr];
	mat4 modelPieceMat = modelMat * pieceMat;

	vec4 vertexPos = vec4(positionAttr, 1.0);
	vec4 vertexShadowPos = shadowViewMat * modelPieceMat * vertexPos;

	gl_Position = shadowProjMat * vertexShadowPos;

	gl_ClipDistance[0] = dot(upperClipPlane, vertexPos);
	gl_ClipDistance[1] = dot(lowerClipPlane, vertexPos);

	vTexCoord = texCoor0Attr;
}

