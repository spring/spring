uniform mat4 shadowViewMat;
uniform mat4 shadowProjMat;
uniform mat4 modelMat;
uniform mat4 pieceMats[128];
uniform vec4 shadowParams;


layout(location = 0) in vec3 positionAttr;
// layout(location = 1) in vec3   normalAttr;
// layout(location = 2) in vec3 sTangentAttr;
// layout(location = 3) in vec3 tTangentAttr;
layout(location = 4) in vec2 texCoor0Attr;
// layout(location = 5) in vec2 texCoor1Attr;
layout(location = 6) in uint pieceIdxAttr;

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
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	gl_Position = shadowProjMat * vertexShadowPos;

	vTexCoord = texCoor0Attr;
}

