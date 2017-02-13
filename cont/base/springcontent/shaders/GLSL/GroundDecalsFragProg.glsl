uniform sampler2D       decalTex;
uniform sampler2D       shadeTex;
uniform sampler2DShadow shadowTex;

uniform vec4 groundAmbientColor;

uniform mat4 shadowMatrix;
uniform vec4 shadowParams;
uniform float shadowDensity;

varying vec4 vertexPos;

void main() {
	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	float shadowCoeff = mix(1.0, shadow2DProj(shadowTex, vertexShadowPos).r, shadowDensity);

	vec4 shadeInt;
	vec4 decalInt;
	vec4 shadeCol;

	#if (HAVE_SHADING_TEX == 1)
	shadeInt = texture2D(shadeTex, gl_TexCoord[1].st);
	#else
	shadeInt = vec4(1.0, 1.0, 1.0, 1.0);
	#endif

	decalInt = texture2D(decalTex, gl_TexCoord[0].st);
	shadeCol = mix(groundAmbientColor, shadeInt, shadowCoeff * shadeInt.a);

	gl_FragColor = decalInt * shadeCol;
	gl_FragColor.a = decalInt.a * gl_Color.a;
}
