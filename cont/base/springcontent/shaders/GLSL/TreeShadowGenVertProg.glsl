#if 0
#define MAX_TREE_HEIGHT 60.0
#endif


uniform mat4 shadowViewMat;
uniform mat4 shadowProjMat;
uniform vec4 shadowParams;

uniform mat4 treeMat;
uniform vec3 cameraDirX;
uniform vec3 cameraDirY;


layout(location = 0) in vec3 vtxPositionAttr; // vertexPosAttr
layout(location = 1) in vec2 vtxTexCoordAttr; // texCoordAttr
layout(location = 2) in vec3 vtxNormalAttr;   // normalVecAttr

out vec2 vTexCoord;
out vec4 vBaseColor;


void main() {
	vec4 vertexPos = vec4(vtxPositionAttr, 1.0);
		vertexPos.xyz += (cameraDirX * vtxNormalAttr.x);
		vertexPos.xyz += (cameraDirY * vtxNormalAttr.y);

	vec4 vertexShadowPos = shadowViewMat * treeMat * vertexPos;

	gl_Position = shadowProjMat * vertexShadowPos;

	#if 0
	vBaseColor.xyz = vtxNormalAttr.z * vec3(1.0, 1.0, 1.0);
	vBaseColor.a = vtxPositionAttr * (0.20 * (1.0 / MAX_TREE_HEIGHT)) + 0.85;
	#endif

	vTexCoord = vtxTexCoordAttr;
}

