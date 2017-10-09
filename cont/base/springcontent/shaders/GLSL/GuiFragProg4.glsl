#version 410 core

uniform sampler2D tex;
uniform vec4 color;
uniform vec4 texWeight;

in vec2 vTexCoord;
out vec4 fOutColor;

void main()
{
	fOutColor = mix(vec4(1.0f), texture(tex, vTexCoord), texWeight) * color;
}

