#version 410 core

uniform sampler2D tex;
uniform vec4 elemColor;
uniform vec4 texWeight;
uniform mat2 texCoorMat;

in vec2 vTexCoord;
in vec4 vFontColor;
out vec4 fFragColor;

const float N = 1.0 / 255.0;

void main()
{
	// background image or font alpha-mask
	vec4 texel = texture(tex, texCoorMat * vTexCoord);

	vec4 aguiColor = mix(vec4(1.0), texel, texWeight) * elemColor;
	vec4 fontColor = (vFontColor * N) * vec4(1.0, 1.0, 1.0, texel.r);

	// texWeight.x is -1 when drawing fonts
	fFragColor = mix(aguiColor, fontColor, float(texWeight.x == -1.0));
}

