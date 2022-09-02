#version 130

uniform sampler2D atlasTex;
uniform vec4 alphaCtrl = vec4(0.0, 1.0, 0.0, 0.0);  //always pass

in vec4 vCol;
in vec3 vUVW;
in vec4 vUVb;
in float vBF;

out vec3 fragColor;

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
	vec4 c0 = texture(atlasTex, vUVb.xy + vUVW.xy);
	vec4 c1 = texture(atlasTex, vUVb.zw + vUVW.xy);

	vec4 color = vec4(mix(c0, c1, vBF));

	color *= vCol;

	if (AlphaDiscard(color.a))
		discard;

	// use "multiply" blending R = S * D, but it needs to account for alpha too
	// so with alpha, this formula seems to make some sense: R = D * (1 - Sa) + S * D * Sa ==>
	// ==> R = D * (1 - Sa + S * Sa)
	// with glBlendFunc(GL_ZERO, GL_SRC_COLOR)
	// GL_SRC_COLOR becomes (1 - Sa + S * Sa), so write this part to the RGB buffer
	fragColor = vec3(1.0) - color.aaa + color.rgb * color.aaa;
}