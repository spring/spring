#if (SUPPORT_DEPTH_LAYOUT == 1)
// preserve early-z performance if possible
layout(depth_unchanged) out float gl_FragDepth;
#endif


// uniform sampler2D alphaMaskTex;

uniform vec4 shadowParams;
// uniform vec2 alphaTestCtrl;

uniform mat4 shadowViewMat;
uniform mat4 shadowProjMat;

#if (SHADOWGEN_PER_FRAGMENT == 1)
in vec4 vertexModelPos;
#endif


void main() {
	#if 0
	// TODO: generate coords and bind this (as for models)
	if (texture2D(alphaMaskTex, vTexCoord).a <= alphaTestCtrl.x)
		discard;
	#endif

	#if (SHADOWGEN_PER_FRAGMENT == 1)
	// note: this voids glPolygonOffset calls
	vec4 vertexShadowPos = shadowViewMat * vertexModelPos;
		vertexShadowPos.z  += 0.00250;
	vec4 vertexShadowClipPos = shadowProjMat * vertexShadowPos;

	float  nearDepth = gl_DepthRange.near;
	float   farDepth = gl_DepthRange.far;
	float depthRange = gl_DepthRange.diff; // far - near
	float  clipDepth = vertexShadowClipPos.z / vertexShadowClipPos.w;

	#if (SUPPORT_CLIP_CONTROL == 1)
	// ZTO; shadow PM is pre-adjusted
	gl_FragDepth = (depthRange * clipDepth) + nearDepth;
	#else
	// NOTO; useful to keep around
	gl_FragDepth = ((depthRange * clipDepth) + nearDepth + farDepth) * 0.5;
	#endif

	#else

	// no-op
	gl_FragDepth = gl_FragCoord.z;
	#endif
}

