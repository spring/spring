#version 150 compatibility

uniform sampler2D atlasTex;
#ifdef SMOOTH_PARTICLES
	uniform sampler2D depthTex;
	uniform vec2 invScreenSize;
	uniform float softenThreshold;
	uniform vec2 softenExponent;
#endif
uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass

in Data{
	vec4 vsPos;
	vec4 vCol;
	vec2 vUV;
};

#define projMatrix gl_ProjectionMatrix


#define NORM2SNORM(value) (value * 2.0 - 1.0)
#define SNORM2NORM(value) (value * 0.5 + 0.5)

#ifdef SMOOTH_PARTICLES
float GetViewSpaceDepth(float d) {
	#ifndef DEPTH_CLIP01
		d = NORM2SNORM(d);
	#endif
	return -projMatrix[3][2] / (projMatrix[2][2] + d);
}
#endif

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
	vec4 color = texture(atlasTex, vUV);
	gl_FragColor  = color * vCol;

	#ifdef SMOOTH_PARTICLES
	vec2 screenUV = gl_FragCoord.xy * invScreenSize;
	float depthZO = texture(depthTex, screenUV).x;
	float depthVS = GetViewSpaceDepth(depthZO);

	if (softenThreshold > 0.0) {
		float edgeSmoothness = smoothstep(0.0, softenThreshold, vsPos.z - depthVS); // soften edges
		gl_FragColor *= pow(edgeSmoothness, softenExponent.x);
	} else {
		float edgeSmoothness = smoothstep(softenThreshold, 0.0, vsPos.z - depthVS); // follow the surface up
		gl_FragColor *= pow(edgeSmoothness, softenExponent.y);
	}
	#endif

	if (AlphaDiscard(gl_FragColor.a))
		discard;
}