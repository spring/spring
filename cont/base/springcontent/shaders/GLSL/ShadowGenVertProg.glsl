uniform vec4 shadowParams;

#ifdef SHADOWGEN_PROGRAM_TREE_NEAR
uniform vec3 cameraDirX;
uniform vec3 cameraDirY;
uniform vec3 treeOffset;

#define MAX_TREE_HEIGHT 60.0
#endif

#ifdef SHADOWGEN_PROGRAM_MAP
varying mat4 shadowViewMat;
varying mat4 shadowProjMat;
varying vec4 vertexModelPos;
#endif


void main() {
	#ifdef SHADOWGEN_PROGRAM_TREE_NEAR
	vec3 offsetVec = treeOffset + (cameraDirX * gl_Normal.x) + (cameraDirY * gl_Normal.y);
	#else
	vec3 offsetVec = vec3(0.0, 0.0, 0.0);
	#endif


	#ifdef SHADOWGEN_PROGRAM_MAP
	shadowViewMat = gl_ModelViewMatrix;
	shadowProjMat = gl_ProjectionMatrix;
	vertexModelPos = gl_Vertex + vec4(offsetVec, 0.0);
	#endif

	vec4 vertexPos = gl_Vertex + vec4(offsetVec, 0.0);
	vec4 vertexShadowPos = gl_ModelViewMatrix * vertexPos;
		vertexShadowPos.xy *= (inversesqrt(abs(vertexShadowPos.xy) + shadowParams.zz) + shadowParams.ww);
		vertexShadowPos.xy += shadowParams.xy;

	gl_Position = gl_ProjectionMatrix * vertexShadowPos;


	#ifdef SHADOWGEN_PROGRAM_MODEL
	gl_TexCoord[0] = gl_MultiTexCoord0;
	#endif

	#ifdef SHADOWGEN_PROGRAM_MAP
	// empty, uses TexGen
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
