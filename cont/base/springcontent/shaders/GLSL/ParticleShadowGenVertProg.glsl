uniform mat4 shadowViewMat;
uniform mat4 shadowProjMat;
uniform vec4 shadowParams;

layout(location = 0) in vec3 vtxPositionAttr;
layout(location = 1) in vec2 vtxTexCoordAttr;
layout(location = 2) in vec4 vtxBaseColorAttr; // non-normalized

out vec2 vTexCoord;
out vec4 vBaseColor;

const float vtxBaseColorMult = 1.0 / 255.0;


void main() {
	vec4 vertexPos = vec4(vtxPositionAttr, 1.0);
	vec4 vertexShadowPos = shadowViewMat * vertexPos;

	gl_Position = shadowProjMat * vertexShadowPos;

	vBaseColor = vtxBaseColorAttr * vtxBaseColorMult;
	vTexCoord = vtxTexCoordAttr;
}

