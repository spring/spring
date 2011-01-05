uniform sampler2D       decalTex;
uniform sampler2D       shadeTex;
uniform sampler2DShadow shadowTex;

uniform vec4 groundAmbientColor;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;
uniform float shadowDensity;

varying vec4 vertexPos;

void main() {
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	float shadowInt = (1.0 - shadow2DProj(shadowTex, vertexShadowPos).r) * shadowDensity;

	vec4 shadeInt;
	vec4 decalInt;
	vec4 shadeCol;

	#if (HAVE_SHADING_TEX == 1)
	shadeInt = texture2D(shadeTex, gl_TexCoord[1].st);
	#else
	shadeInt = vec4(1.0, 1.0, 1.0, 1.0);
	#endif

	shadowInt = 1.0 - (shadowInt * shadeInt.a);
	decalInt = texture2D(decalTex, gl_TexCoord[0].st);
	shadeCol = mix(groundAmbientColor, shadeInt, shadowInt);

	gl_FragColor = decalInt * shadeCol;
	gl_FragColor.a = decalInt.a * gl_Color.a;
}
