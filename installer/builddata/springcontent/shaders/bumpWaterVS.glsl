/**
 * @project Spring RTS
 * @file bumpWaterVS.glsl
 * @brief An extended bumpmapping water shader
 * @author jK
 *
 * Copyright (C) 2008.  Licensed under the terms of the
 * GNU GPL, v2 or later.
 */

//////////////////////////////////////////////////
// runtime defined constants are:
// #define SurfaceColor     vec4
// #define DiffuseColor     vec3
// #define PlaneColor       vec4  (unused)
// #define AmbientFactor    float
// #define DiffuseFactor    float   (note: it is the map defined value multipled with 15x!)
// #define SpecularColor    vec3
// #define SpecularPower    float
// #define SpecularFactor   float
// #define PerlinStartFreq  float
// #define PerlinFreq       float
// #define PerlinAmp        float
// #define Speed            float
// #define FresnelMin       float
// #define FresnelMax       float
// #define FresnelPower     float
// #define ScreenInverse    vec2
// #define ViewPos          vec2
// #define MapMid           vec3
// #define SunDir           vec3
// #define ReflDistortion   float
// #define BlurBase         vec2
// #define BlurExponent     float
// #define PerlinStartFreq  float
// #define PerlinLacunarity float
// #define PerlinAmp        float
// #define TexGenPlane      vec4

//////////////////////////////////////////////////
// possible flags are:
// //#define use_heightmap
// #define use_reflection
// #define use_refraction
// #define use_shorewaves
// #define use_depth
// #define blur_reflection
// #define use_texrect

#define Speed 12.0

uniform float frame;
uniform vec3 eyePos;
varying vec3 eyeVec;
varying vec3 ligVec;

void main(void)
{
	//gl_Position = ftransform();

	// COMPUTE TEXCOORDS
	gl_TexCoord[0] = TexGenPlane*gl_Vertex.xzxz;
	gl_TexCoord[4].pq = ShadingPlane.xy*gl_Vertex.xz;

	// COMPUTE WAVE TEXTURE COORDS
	const float fstart = PerlinStartFreq;
	const float f      = PerlinLacunarity;
	gl_TexCoord[1].st = (vec2(-1.0,-1.0) + gl_TexCoord[0].pq + 0.75) * fstart       + frame * Speed;
	gl_TexCoord[1].pq = (vec2(-1.0, 1.0) + gl_TexCoord[0].pq + 0.50) * fstart*f     - frame * Speed;
	gl_TexCoord[2].st = (vec2( 1.0,-1.0) + gl_TexCoord[0].pq + 0.25) * fstart*f*f   + frame * Speed * vec2(1.0,-1.0);
	gl_TexCoord[2].pq = (vec2( 1.0, 1.0) + gl_TexCoord[0].pq + 0.00) * fstart*f*f*f + frame * Speed * vec2(-1.0,1.0);
	gl_TexCoord[3].st = gl_TexCoord[0].pq * 160.0 + frame;
	gl_TexCoord[3].pq = gl_TexCoord[0].pq * 90.0  + frame;
	gl_TexCoord[4].st = gl_TexCoord[0].pq * 2.0;

	// COMPUTE LIGHT VECTORS
	eyeVec = eyePos - gl_Vertex.xyz;
	ligVec = normalize(SunDir*20000.0 + MapMid - gl_Vertex.xyz);

	// FOG
	gl_FogFragCoord = (gl_ModelViewMatrix*gl_Vertex).z;

	gl_Vertex.y += 3.0 * (cos(frame * 500.0 + gl_Vertex.z) * sin(frame * 500.0 + gl_Vertex.x / 1000.0));
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
}
