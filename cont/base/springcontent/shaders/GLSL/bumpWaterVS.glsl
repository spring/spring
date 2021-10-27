/**
 * @project Spring RTS
 * @file bumpWaterVS.glsl
 * @brief An extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008,2009.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

//////////////////////////////////////////////////
// runtime defined constants are:
// #define SurfaceColor       vec4
// #define DiffuseColor       vec3
// #define PlaneColor         vec4  (unused)
// #define AmbientFactor      float
// #define DiffuseFactor      float   (note: it is the map defined value multipled with 15x!)
// #define SpecularColor      vec3
// #define SpecularPower      float
// #define SpecularFactor     float
// #define PerlinStartFreq    float
// #define PerlinFreq         float
// #define PerlinAmp          float
// #define Speed              float
// #define FresnelMin         float
// #define FresnelMax         float
// #define FresnelPower       float
// #define ScreenInverse      vec2
// #define ViewPos            vec2
// #define MapMid             vec3
// #define SunDir             vec3
// #define ReflDistortion     float
// #define BlurBase           vec2
// #define BlurExponent       float
// #define PerlinStartFreq    float
// #define PerlinLacunarity   float
// #define PerlinAmp          float
// #define WindSpeed          float
// #define WaveOffsetFactor   float
// #define WaveLength         float
// #define WaveFoamDistortion float
// #define WaveFoamIntensity  float
// #define CausticsResolution float
// #define CausticsStrength   float

// #define TexGenPlane      vec4
// #define ShadingPlane     vec4

//////////////////////////////////////////////////
// possible flags are:
// //#define opt_heightmap
// #define opt_reflection
// #define opt_refraction
// #define opt_shorewaves
// #define opt_depth
// #define opt_blurreflection
// #define opt_texrect
// #define opt_endlessocean

uniform float frame;
uniform vec3 eyePos;

varying float eyeVertexZ;
varying vec3 eyeVec;
varying vec3 ligVec;
varying vec3 worldPos;

void main()
{
	// COMPUTE TEXCOORDS
	gl_TexCoord[0] = TexGenPlane * gl_Vertex.xzxz;
	gl_TexCoord[5].st = ShadingPlane.xy * gl_Vertex.xz;

	// COMPUTE WAVE TEXTURE COORDS
	float fstart = PerlinStartFreq;
	float f      = PerlinLacunarity;
	gl_TexCoord[1].st = (vec2(-1.0,-1.0) + gl_TexCoord[0].pq + 0.75) * fstart       + frame * WindSpeed * gl_MultiTexCoord1.st;
	gl_TexCoord[1].pq = (vec2(-1.0, 1.0) + gl_TexCoord[0].pq + 0.50) * fstart*f     - frame * WindSpeed * gl_MultiTexCoord1.st;
	gl_TexCoord[2].st = (vec2( 1.0,-1.0) + gl_TexCoord[0].pq + 0.25) * fstart*f*f   + frame * WindSpeed * gl_MultiTexCoord1.st;
	gl_TexCoord[2].pq = (vec2( 1.0, 1.0) + gl_TexCoord[0].pq + 0.00) * fstart*f*f*f + frame * WindSpeed * gl_MultiTexCoord1.st;

	gl_TexCoord[3].st = gl_TexCoord[0].pq * 160.0 + frame * 2.5;
	gl_TexCoord[3].pq = gl_TexCoord[0].pq * 90.0  - frame * 2.0;
	gl_TexCoord[4].st = gl_TexCoord[0].pq * 2.0;
	gl_TexCoord[4].pq = gl_TexCoord[0].pq * 6.0 + frame * 0.37;

	// SIMULATE WAVES
	// TODO:
	//   restrict amplitude to less than shallow water depth
	//   decrease cycling speed of caustics when zoomed out?
	vec4 waveVertex;
	waveVertex.xzw = gl_Vertex.xzw;
	waveVertex.y = 3.0 * (cos(frame * 500.0 + gl_Vertex.z) * sin(frame * 500.0 + gl_Vertex.x / 1000.0));

	// COMPUTE LIGHT VECTORS
	eyeVec = eyePos - waveVertex.xyz;
	ligVec = normalize(SunDir * 20000.0 + MapMid - waveVertex.xyz);

	// FOG
	worldPos = waveVertex.xyz;
	gl_FogFragCoord = (gl_ModelViewMatrix * waveVertex).z;

	gl_Position = gl_ModelViewProjectionMatrix * waveVertex;

	#if 0
	// distance to unperturbed vertex
	eyeVertexZ = (gl_ModelViewMatrix * gl_Vertex).z;
	#endif
}

