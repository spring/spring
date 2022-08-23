#if (GL_FRAGMENT_PRECISION_HIGH == 1)
// ancient GL3 ATI drivers confuse GLSL for GLSL-ES and require this
precision highp float;
#else
precision mediump float;
#endif

#if (GL_ARB_conservative_depth == 1 && SUPPORT_DEPTH_LAYOUT == 1)
	layout(depth_unchanged) out float gl_FragDepth;
#endif

uniform sampler2D alphaMaskTex;
uniform vec2 alphaParams;

out vec4 fragColor;

void main() {
	#if 0
	// TODO: bind this (as for models)
	if (texture2D(alphaMaskTex, gl_TexCoord[0].st).a <= alphaParams.x)
		discard;
	#endif
}
