#version 410 core

const float BASE_COLOR_MULT = 1.0 / 255.0;

uniform sampler2D heightTex;

uniform mat4 viewMatrix;
uniform mat4 projMatrix;
uniform mat4 quadMatrix;

// .xy := 1.0f / (pwr2map{x,z} * SQUARE_SIZE)
// .zw := 1.0f / (    map{x,z} * SQUARE_SIZE)
uniform vec4 invMapSize;


layout(location = 0) in vec3 vertexPosAttr;
layout(location = 1) in vec2 texCoordsAttr;
layout(location = 2) in vec4 baseColorAttr;

out vec4 vertexPos;
out vec2 texCoord0;
out vec2 texCoord1;
out vec4 baseColor;


void main() {
	vertexPos = vec4(vertexPosAttr, 1.0);
	vertexPos.y = texture(heightTex, vertexPosAttr.xz * invMapSize.zw).r + 0.05;

	gl_Position = projMatrix * viewMatrix * quadMatrix * vertexPos;

	texCoord0 = texCoordsAttr;
	texCoord1 = vertexPosAttr.xz * invMapSize.xy;
	baseColor = baseColorAttr * BASE_COLOR_MULT;
}

