#version 410 core

layout(location = 0) in vec3 aVertexPos;
layout(location = 1) in vec2 aTexCoord;
layout(location = 2) in vec4 aFontColor; // used only in font-mode; has to be #2

out vec2 vTexCoord;
out vec4 vFontColor;

uniform mat4 viewProjMat;

void main()
{
	gl_Position = viewProjMat * vec4(aVertexPos, 1.0);

	vTexCoord = aTexCoord;
	vFontColor = aFontColor;
}

