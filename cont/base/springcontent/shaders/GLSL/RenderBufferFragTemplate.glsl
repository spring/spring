#version 150

uniform sampler2D tex;
uniform vec4 ucolor = vec4(1.0);

// FS input attributes
%s

// FS output attributes
out vec4 outColor;

uniform vec4 alphaCtrl = vec4(0.0, 0.0, 0.0, 1.0); //always pass

bool AlphaDiscard(float a) {
	float alphaTestGT = float(a > alphaCtrl.x) * alphaCtrl.y;
	float alphaTestLT = float(a < alphaCtrl.x) * alphaCtrl.z;

	return ((alphaTestGT + alphaTestLT + alphaCtrl.w) == 0.0);
}

void main() {
%s
	outColor *= ucolor;
	if (AlphaDiscard(outColor.a))
		discard;
	
}