#version 150 compatibility

uniform sampler2D atlasTex;
uniform vec4 alphaCtrl = vec4(0.3, 1.0, 0.0, 0.0);

in Data{
	vec4 vCol;
	vec2 vUV;
};

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
	vec4 color = texture(atlasTex, vUV);
	gl_FragColor  = color * vCol;

	if (AlphaDiscard(gl_FragColor.a))
		discard;
}