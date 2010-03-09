#if (defined(TREE_BASIC) || defined(TREE_NEAR))
uniform vec3 cameraDirX;
uniform vec3 cameraDirY;
uniform vec3 treeOffset;

uniform vec4 groundDiffuseColor;
uniform vec4 groundAmbientColor;
#endif

#ifdef TREE_BASIC
uniform vec4 invMapSizePO2;
#endif

#if (defined(TREE_NEAR) || defined(TREE_DIST))
uniform mat4 shadowMatrix;
uniform vec4 shadowParams;
#endif

#define MAX_TREE_HEIGHT 60.0

void main() {
	vec4 vertexPos = gl_Vertex;

	#if (defined(TREE_BASIC) || defined(TREE_NEAR))
	vertexPos.xyz += treeOffset;
	vertexPos.xyz += (cameraDirX * gl_Normal.x);
	vertexPos.xyz += (cameraDirY * gl_Normal.y);

	gl_FrontColor.rgb = (gl_Normal.z * groundDiffuseColor.rgb) + groundAmbientColor.rgb;
	gl_FrontColor.a = (gl_Vertex.y * (0.20 * (1.0 / MAX_TREE_HEIGHT))) + groundDiffuseColor.w;
	#endif
	#if (defined(TREE_DIST)
	gl_FrontColor = gl_Color;
	#endif

	#if (defined(TREE_NEAR) || defined(TREE_DIST))
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;
	#endif

	#ifdef TREE_BASIC
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = vertexPos.xzyw * invMapSizePO2;
	#endif
	#ifdef TREE_NEAR
	gl_TexCoord[0] = vertexShadowPos;
	gl_TexCoord[1] = gl_MultiTexCoord1;
	#endif
	#ifdef TREE_DIST
	gl_TexCoord[0] = vertexShadowPos;
	gl_TexCoord[1] = gl_MultiTexCoord0;
	#endif

	gl_FogFragCoord = gl_ModelViewProjectionMatrix[2] * vertexPos;
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;
}
