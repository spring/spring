#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif



#ifdef SHADOWGEN_PROGRAM_MAP

#if (SUPPORT_DEPTH_LAYOUT == 1)
// preserve early-z performance if possible
layout(depth_unchanged) out float gl_FragDepth;
#endif

// uniform sampler2D alphaMaskTex;

uniform vec4 shadowParams;
// uniform vec2 alphaParams;

in mat4 shadowViewMat;
in mat4 shadowProjMat;
in vec4 vertexModelPos;

void main() {
	#if 0
	// TODO: bind this (as for models)
	if (texture2D(alphaMaskTex, gl_TexCoord[0].st).a <= alphaParams.x)
		discard;
	#endif

	#if (SHADOWGEN_PER_FRAGMENT == 1)
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


#else


uniform sampler2D alphaMaskTex;

// uniform vec4 shadowParams;
uniform vec2 alphaParams;

void main() {
	if (texture2D(alphaMaskTex, gl_TexCoord[0].st).a <= alphaParams.x)
		discard;

	gl_FragDepth = gl_FragCoord.z;
}

#endif

