#version 410 core

layout(location = 0) in vec3 aVertexPos;
layout(location = 1) in vec4 aTexCoord;

uniform mat4 uViewMat;
uniform mat4 uProjMat;

out vec4 vTexCoord;

void main() {
	vTexCoord = aTexCoord;
	gl_Position = uProjMat * uViewMat * vec4(aVertexPos, 1.0);
}

