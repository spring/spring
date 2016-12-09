uniform sampler2D alphaMaskTex;

varying vec4 vertexShadowClipPos;

void main() {
	if (texture2D(alphaMaskTex, gl_TexCoord[0].st).a <= 0.5)
		discard;

	#if 0
	float  nearDepth = gl_DepthRange.near;
	float   farDepth = gl_DepthRange.far;
	float depthRange = gl_DepthRange.diff; // far - near
	float  clipDepth = vertexShadowClipPos.z / vertexShadowClipPos.w;

	// useful to keep around
	gl_FragDepth = ((depthRange * clipDepth) + nearDepth + farDepth) * 0.5;

	#else

	// no-op, but breaks early-z
	gl_FragDepth = gl_FragCoord.z;
	#endif
}

