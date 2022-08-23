#version 120

// note: gl_ModelViewMatrix actually only contains the
// model matrix, view matrix is on the projection stack

varying vec4 vertexWorldPos;
varying vec3 cameraDir;
varying float fogFactor;

varying vec3 normalv;

#if (USE_SHADOWS == 1)
	uniform mat4 shadowMatrix;
	varying vec4 shadowVertexPos;
#endif

void main(void)
{
	normalv = gl_NormalMatrix * gl_Normal;

	gl_ClipVertex  = gl_ModelViewMatrix * gl_Vertex; // M (!)
	gl_Position    = gl_ProjectionMatrix * gl_ClipVertex;

	vertexWorldPos = gl_ClipVertex;

	vec4 cameraPos = gl_ProjectionMatrixInverse * vec4(0, 0, 0, 1); cameraPos.xyz /= cameraPos.w;

	cameraDir      = vertexWorldPos.xyz - cameraPos.xyz;

#if (USE_SHADOWS == 1)
	shadowVertexPos = shadowMatrix * vertexWorldPos;
	shadowVertexPos.xy += vec2(0.5);
#endif

	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

#if (DEFERRED_MODE == 0)
	float fogCoord = length(cameraDir.xyz);
	fogFactor = (gl_Fog.end - fogCoord) * gl_Fog.scale; //gl_Fog.scale := 1.0 / (gl_Fog.end - gl_Fog.start)
	fogFactor = clamp(fogFactor, 0.0, 1.0);
#endif
}
