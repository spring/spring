#version 130

uniform sampler2D atlasTex;
uniform sampler2D depthTex;

in vec4 vertColor;
in vec4 vsPos;

#define projMatrix gl_ProjectionMatrix

uniform vec2 invScreenRes;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

float GetViewSpaceDepth(float depthNDC) {
	return -projMatrix[3][2] / (projMatrix[2][2] + depthNDC);
}

float GetViewSpaceDepthFromTexture(float depthNDC) {
	#ifndef DEPTH_CLIP01
		depthNDC = NORM2SNORM(depthNDC);
	#endif
	return GetViewSpaceDepth(depthNDC);
}

void main() {
	vec4 color = texture(atlasTex, gl_TexCoord[0].xy);
	float depthVS = GetViewSpaceDepthFromTexture(texture(depthTex, gl_FragCoord.xy * invScreenRes).x);
	float edgeSmoothness = smoothstep(0.0, 5.0, vsPos.z - depthVS);
	gl_FragColor = color * vertColor;
	gl_FragColor *= edgeSmoothness;
}