#version 410 core

layout(location = 0) in vec2 aVertexPos;
layout(location = 1) in vec2 aTexCoord;

out vec2 vTexCoord;

uniform mat4 viewProjMatrix;

void main()
{
	gl_Position = viewProjMatrix * vec4(aVertexPos, 0.0, 1.0);
	vTexCoord = aTexCoord;
}

