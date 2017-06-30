uniform sampler2D alphaMaskTex;

uniform vec4 shadowParams;
uniform vec2 alphaParams;

varying mat4 shadowViewMat;
varying mat4 shadowProjMat;
varying vec4 vertexModelPos;


void main() {
	#if 0
	// TODO: bind this (as for models)
	if (texture2D(alphaMaskTex, gl_TexCoord[0].st).a <= alphaParams.x)
		discard;
	#endif

	#if 1
	// note: voids glPolygonOffset calls
	vec4 vertexShadowPos = shadowViewMat * vertexModelPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;
		vertexShadowPos.z  += 0.00250;
	vec4 vertexShadowClipPos = shadowProjMat * vertexShadowPos;

	float  nearDepth = gl_DepthRange.near;
	float   farDepth = gl_DepthRange.far;
	float depthRange = gl_DepthRange.diff; // far - near
	float  clipDepth = vertexShadowClipPos.z / vertexShadowClipPos.w;

	#if (SUPPORT_CLIP_CONTROL == 1)
	// shadow PM is pre-adjusted
	gl_FragDepth = (depthRange * clipDepth) + nearDepth;
	#else
	// useful to keep around
	gl_FragDepth = ((depthRange * clipDepth) + nearDepth + farDepth) * 0.5;
	#endif

	#else

	// no-op, but breaks early-z
	gl_FragDepth = gl_FragCoord.z;
	#endif
}
