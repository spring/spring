uniform vec4 shadowParams;


void main() {
	vec4 vertexPos = gl_Vertex;
	vec4 vertexShadowPos = gl_ModelViewMatrix * vertexPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	gl_Position = gl_ProjectionMatrix * vertexShadowPos;


	#ifdef SHADOWGEN_PROGRAM_MODEL
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif
}

