// note: gl_ModelViewMatrix actually only contains the
// model matrix, view matrix is on the projection stack
//
// todo: clip gl_Position against gl_ClipPlane[3] if advFade
// note: many gfx will fallback to software rendering when
// gl_ClipDistance or gl_ClipPosition are used, so it might
// be better to use may a `discard` in the fragment shader
//
// note: shadow-map texture coordinates should be generated
// per fragment (the non-linear projection used can produce
// shifting artefacts with large triangles due to the linear
// interpolation of vertex positions), but this is a source
// of acne itself


//#define use_normalmapping
//#define flip_normalmap
#define use_shadows

  //uniform mat4 cameraMat;
  //uniform mat4 cameraInv;
  uniform vec3 cameraPos;
#ifdef use_shadows
  uniform mat4 shadowMatrix;
  uniform vec4 shadowParams;
#endif

  varying vec3 cameraDir;
  varying float fogFactor;

#ifdef use_normalmapping
  varying mat3 tbnMatrix;
#else
  varying vec3 normalv;
#endif

void main(void)
{
#ifdef use_normalmapping
	vec3 tangent   = gl_MultiTexCoord5.xyz;
	vec3 bitangent = gl_MultiTexCoord6.xyz;
	tbnMatrix      = gl_NormalMatrix * mat3(tangent, bitangent, gl_Normal);
#else
	normalv = gl_NormalMatrix * gl_Normal;
#endif

	vec4 worldPos = gl_ModelViewMatrix * gl_Vertex;
	gl_Position   = gl_ProjectionMatrix * worldPos;
	cameraDir     = worldPos.xyz - cameraPos;

#ifdef use_shadows
	gl_TexCoord[1] = shadowMatrix * worldPos;
	gl_TexCoord[1].st = gl_TexCoord[1].st * (inversesqrt( abs(gl_TexCoord[1].st) + shadowParams.z) + shadowParams.w) + shadowParams.xy;
#endif

	gl_TexCoord[0].st = gl_MultiTexCoord0.st;

	float fogCoord = length(gl_Position.xyz);
	fogFactor = (gl_Fog.end - fogCoord) * gl_Fog.scale; //gl_Fog.scale := 1.0 / (gl_Fog.end - gl_Fog.start)
	fogFactor = clamp(fogFactor, 0.0, 1.0);
}
