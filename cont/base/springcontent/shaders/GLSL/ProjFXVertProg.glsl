#version 150 compatibility
//#extension GL_ARB_explicit_attrib_location : require

in vec3 pos;
in vec2 uv;
in vec4 col;
/*
layout (location = 0) in vec3 pos;
layout (location = 1) in vec2 uv;
layout (location = 2) in vec4 col;
*/

out Data {
	vec4 vsPos;
	vec4 vCol;
	vec2 vUV;
};

void main() {
	vCol = col;
	vUV  = uv;
	vsPos = gl_ModelViewMatrix * vec4(pos, 1.0);
	gl_Position = gl_ProjectionMatrix * vsPos;
}