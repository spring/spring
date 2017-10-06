#version 130

uniform sampler2D tex;
uniform vec4 color;
uniform vec4 texWeight;

out vec4 outColor;

void main()
{
	outColor = mix(vec4(1.0f), texture2D(tex, gl_TexCoord[0].st), texWeight) * color;
}

