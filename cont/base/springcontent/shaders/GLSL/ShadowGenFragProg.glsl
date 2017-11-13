uniform sampler2D alphaMaskTex;

uniform vec2 alphaParams;


void main() {
	if (texture2D(alphaMaskTex, gl_TexCoord[0].st).a <= alphaParams.x)
		discard;

	gl_FragDepth = gl_FragCoord.z;
}

