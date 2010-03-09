uniform vec4 shadowParams;   // {x = xmid, y = ymid, z = p17, w = p18}

#ifdef SHADOWGEN_PROGRAM_TREE_NEAR
uniform vec3 cameraDirX;
uniform vec3 cameraDirY;
uniform vec3 treeOffset;

#define MAX_TREE_HEIGHT 60.0
#endif


void main() {
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexPos = gl_Vertex;

	#ifdef SHADOWGEN_PROGRAM_TREE_NEAR
	vertexPos.xyz += treeOffset;
	vertexPos.xyz += (cameraDirX * gl_Normal.x);
	vertexPos.xyz += (cameraDirY * gl_Normal.y);
	#endif

	vec4 vertexShadowPos = gl_ModelViewMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;

	gl_Position = gl_ProjectionMatrix * vertexShadowPos;

	#ifdef SHADOWGEN_PROGRAM_MODEL
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_MAP
	// empty
	#endif

	#ifdef SHADOWGEN_PROGRAM_TREE_NEAR
	gl_FrontColor.xyz = gl_Normal.z * vec3(1.0, 1.0, 1.0);
	gl_FrontColor.a = gl_Vertex.y * (0.20 * (1.0 / MAX_TREE_HEIGHT)) + 0.85;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_TREE_DIST
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_PROJECTILE
	gl_FrontColor = gl_Color;
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif
}
