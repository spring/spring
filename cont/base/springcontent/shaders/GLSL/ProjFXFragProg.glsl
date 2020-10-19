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

//very slow due to no early-z test
void HighQualitySoften(float depthZO, vec4 color) {
	float depthVS = GetViewSpaceDepth(depthZO);

	float edgeSmoothness = smoothstep(-16.0, 0.0, vsPos.z - depthVS);

	gl_FragColor = color * vertColor;
	gl_FragColor *= edgeSmoothness;
	float fragDepth = mix(depthZO - 1e-3, gl_FragCoord.z, 1.0 - edgeSmoothness);
	gl_FragDepth = fragDepth; //always 0 to -1, no matter clip_ctrl
}

void NormQualitySoften(float depthZO, vec4 color) {
	float depthVS = GetViewSpaceDepth(depthZO);

	float edgeSmoothness = smoothstep(0.0, 16.0, vsPos.z - depthVS);

	gl_FragColor = color * vertColor;
	gl_FragColor *= edgeSmoothness;
}

void main() {
	vec4 color = texture(atlasTex, gl_TexCoord[0].xy);
	vec2 screenUV = gl_FragCoord.xy * invScreenSize;

	float depthZO = texture(depthTex, screenUV).x;
	NormQualitySoften(depthZO, color);
}