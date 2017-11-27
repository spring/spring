#version 410 core

const float BASE_COLOR_MULT = 1.0 / 255.0;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 quadMatrix;
uniform vec2 mapSizePO2; // 1.0f / (pwr2map{x,z} * SQUARE_SIZE)


layout(location = 0) in vec3 vertexPosAttr;
layout(location = 1) in vec2 texCoordsAttr;
layout(location = 2) in vec4 baseColorAttr;

out vec4 vertexPos;
out vec2 texCoord0;
out vec2 texCoord1;
out vec4 baseColor;


void main() {
	vertexPos = vec4(vertexPosAttr, 1.0);

	gl_Position = projMatrix * viewMatrix * quadMatrix * vertexPos;

	texCoord0 = texCoordsAttr;
	texCoord1 = vertexPosAttr.xz * mapSizePO2;
	baseColor = baseColorAttr * BASE_COLOR_MULT;
}

