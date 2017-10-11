#version 410 core

#define SMF_TEXSQR_SIZE 1024.0
#define SMF_DETAILTEX_RES 0.02

uniform ivec2 texSquare;
uniform mat4 viewMat;
uniform mat4 viewMatInv;
uniform mat4 viewProjMat;

layout(location = 0) in vec3 vertexWorldPosIn;

out vec4 vertexWorldPos;

void main() {
	vertexWorldPos = vec4(vertexWorldPosIn, 1.0);

	gl_Position = viewProjMat * vec4(vertexWorldPosIn, 1.0);
	// TODO: reimplement
	// gl_ClipVertex = viewMat * vec4(vertexWorldPosIn, 1.0);
	vec4 clipVertex = viewMat * vec4(vertexWorldPosIn, 1.0);
}

