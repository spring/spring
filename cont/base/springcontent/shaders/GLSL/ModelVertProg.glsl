#version 120

// note: gl_ModelViewMatrix actually only contains the
// model matrix, view matrix is on the projection stack

// #define use_normalmapping
// #define flip_normalmap

  //uniform mat4 cameraMat;
  //uniform mat4 cameraInv;
  uniform vec3 cameraPos;

  varying vec4 vertexWorldPos;
  varying vec3 cameraDir;
  varying float fogFactor;

#ifdef use_normalmapping
  varying mat3 tbnMatrix;
#else
  varying vec3 normalv;
#endif

uniform int numModelDynLights;


void main(void)
{
#ifdef use_normalmapping
	vec3 tangent   = gl_MultiTexCoord5.xyz;
	vec3 bitangent = gl_MultiTexCoord6.xyz;
	tbnMatrix      = gl_NormalMatrix * mat3(tangent, bitangent, gl_Normal);
#else
	normalv = gl_NormalMatrix * gl_Normal;
#endif

	gl_ClipVertex  = gl_ModelViewMatrix * gl_Vertex; // M (!)
	gl_Position    = gl_ProjectionMatrix * gl_ClipVertex;

	vertexWorldPos = gl_ClipVertex;
	cameraDir      = vertexWorldPos.xyz - cameraPos;

	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

	#if (DEFERRED_MODE == 0)
	float fogCoord = length(cameraDir.xyz);
	fogFactor = (gl_Fog.end - fogCoord) * gl_Fog.scale; //gl_Fog.scale := 1.0 / (gl_Fog.end - gl_Fog.start)
	fogFactor = clamp(fogFactor, 0.0, 1.0);
	#endif
}
