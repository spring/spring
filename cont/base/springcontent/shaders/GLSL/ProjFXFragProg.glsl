#version 130

uniform sampler2D atlasTex;
uniform sampler2D depthTex;

in vec4 vertColor;
in vec4 vsPos;

#define projMatrix gl_ProjectionMatrix

uniform vec2 invScreenSize;

#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

float GetViewSpaceDepth(float d) {
	#ifndef DEPTH_CLIP01
		d = NORM2SNORM(d);
	#endif
	return -projMatrix[3][2] / (projMatrix[2][2] + d);
}

void main() {
	vec4 color = texture(atlasTex, gl_TexCoord[0].xy);
	vec2 screenUV = gl_FragCoord.xy * invScreenSize;

	float depthZO = texture(depthTex, screenUV).x;
	float depthVS = GetViewSpaceDepth(depthZO);

	float edgeSmoothness = smoothstep(-16.0, 0.0, vsPos.z - depthVS);

	gl_FragColor = color * vertColor;
	gl_FragColor *= edgeSmoothness;
	gl_FragDepth = min(gl_FragCoord.z, depthZO - 1e-3); //always 0 to -1, no matter clip_ctrl
}