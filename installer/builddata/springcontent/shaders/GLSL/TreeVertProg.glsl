#if (defined(TREE_NEAR_BASIC) || defined(TREE_NEAR_SHADOW))
uniform vec3 cameraDirX;
uniform vec3 cameraDirY;
uniform vec3 treeOffset;

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;

uniform vec2 alphaModifiers;      // (tree-height alpha, ground-diffuse alpha)
#endif

#ifdef TREE_NEAR_BASIC
uniform vec4 invMapSizePO2;
#endif

#if (defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
uniform mat4 shadowMatrix;
uniform vec4 shadowParams;

varying float fogFactor;
#endif

#define MAX_TREE_HEIGHT 60.0

void main() {
	vec4 vertexPos = gl_Vertex;

	#if (defined(TREE_NEAR_BASIC) || defined(TREE_NEAR_SHADOW))
	vertexPos.xyz += treeOffset;
	vertexPos.xyz += (cameraDirX * gl_Normal.x);
	vertexPos.xyz += (cameraDirY * gl_Normal.y);

	gl_FrontColor.rgb = (gl_Normal.z * groundDiffuseColor.rgb) + groundAmbientColor.rgb;
	gl_FrontColor.a = (gl_Vertex.y * alphaModifiers.x) + alphaModifiers.y;
	#endif
	#if (defined(TREE_DIST_SHADOW))
	gl_FrontColor = gl_Color;
	#endif

	#if (defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
	vec2 p17 = vec2(shadowParams.z, shadowParams.z);
	vec2 p18 = vec2(shadowParams.w, shadowParams.w);

	vec4 vertexShadowPos = shadowMatrix * vertexPos;
		vertexShadowPos.st *= (inversesqrt(abs(vertexShadowPos.st) + p17) + p18);
		vertexShadowPos.st += shadowParams.xy;
	#endif

	#if (defined(TREE_NEAR_BASIC))
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = vertexPos.xzyw * invMapSizePO2;
	#endif
	#if (defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
	gl_TexCoord[0] = vertexShadowPos;
	gl_TexCoord[1] = gl_MultiTexCoord0;
	#endif

	gl_FogFragCoord = length((gl_ModelViewMatrix * vertexPos).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;

	#if (defined(TREE_NEAR_SHADOW) || defined(TREE_DIST_SHADOW))
	fogFactor = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	#endif
}
