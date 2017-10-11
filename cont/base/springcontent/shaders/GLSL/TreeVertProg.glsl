#if (defined(TREE_BASIC) || defined(TREE_SHADOW))
uniform vec3 cameraDirX;
uniform vec3 cameraDirY;
uniform vec3 treeOffset;

uniform vec3 groundAmbientColor;
uniform vec3 groundDiffuseColor;

uniform vec2 alphaModifiers;      // (tree-height alpha, ground-diffuse alpha)
#endif

#ifdef TREE_BASIC
uniform vec4 invMapSizePO2;
#endif

#if (defined(TREE_SHADOW))
varying float fogFactor;
#endif

#define MAX_TREE_HEIGHT 60.0

void main() {
	vec4 vertexPos = gl_Vertex;

	#if (defined(TREE_BASIC) || defined(TREE_SHADOW))
	vertexPos.xyz += treeOffset;
	vertexPos.xyz += (cameraDirX * gl_Normal.x);
	vertexPos.xyz += (cameraDirY * gl_Normal.y);

	gl_FrontColor.rgb = (gl_Normal.z * groundDiffuseColor.rgb) + groundAmbientColor.rgb;
	gl_FrontColor.a = (gl_Vertex.y * alphaModifiers.x) + alphaModifiers.y;
	#endif

	#if (defined(TREE_BASIC))
	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_TexCoord[1] = vertexPos.xzyw * invMapSizePO2;
	#endif

	#if (defined(TREE_SHADOW))
	gl_TexCoord[2] = vertexPos;
	gl_TexCoord[1] = gl_MultiTexCoord0;
	#endif


	gl_FogFragCoord = length((gl_ModelViewMatrix * vertexPos).xyz);
	gl_Position = gl_ModelViewProjectionMatrix * vertexPos;

	#if (defined(TREE_SHADOW))
	fogFactor = (gl_Fog.end - gl_FogFragCoord) / (gl_Fog.end - gl_Fog.start);
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	#endif
}
