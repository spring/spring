uniform vec4 shadowParams;   // {x = xmid, y = ymid, z = p17, w = p18}

void main() {
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = gl_ModelViewMatrix * gl_Vertex;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	gl_Position = gl_ProjectionMatrix * vertexShadowPos;

	#ifdef SHADOWGEN_PROGRAM_MODEL
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_MAP
	#endif

	#ifdef SHADOWGEN_PROGRAM_TREE_NEAR
	// TODO
	// gl_FrontColor.xyz = env11 * gl_Normal.z;
	// gl_FrontColor.a = env12.w * gl_Vertex.y + env11.w;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_TREE_FAR
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_PROJECTILE
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif
}
