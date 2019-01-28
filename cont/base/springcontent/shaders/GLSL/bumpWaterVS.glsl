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
// #define WindSpeed        float
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

uniform mat4 projMatrix;
uniform mat4 viewMatrix;

uniform vec3 eyePos;
uniform vec2 windVector;


layout(location = 0) in vec3 vertexPosAttr;

out vec4 texCoord0;
out vec4 texCoord1;
out vec4 texCoord2;
out vec4 texCoord3;
out vec4 texCoord4;
out vec4 texCoord5;

out vec3 eyeVec;
out vec3 sunVec;
out vec3 worldPos;

out float fogCoord;


void main()
{
	vec4 vertexPos = vec4(vertexPosAttr, 1.0);

	// COMPUTE TEXCOORDS
	texCoord0 = TexGenPlane * vertexPos.xzxz;
	texCoord5.st = ShadingPlane.xy * vertexPos.xz;

	// COMPUTE WAVE TEXTURE COORDS
	float fstart = PerlinStartFreq;
	float f      = PerlinLacunarity;

	texCoord1.st = (vec2(-1.0, -1.0) + texCoord0.pq + 0.75) * fstart       + frame * WindSpeed * windVector;
	texCoord1.pq = (vec2(-1.0,  1.0) + texCoord0.pq + 0.50) * fstart*f     - frame * WindSpeed * windVector;
	texCoord2.st = (vec2( 1.0, -1.0) + texCoord0.pq + 0.25) * fstart*f*f   + frame * WindSpeed * windVector;
	texCoord2.pq = (vec2( 1.0,  1.0) + texCoord0.pq + 0.00) * fstart*f*f*f + frame * WindSpeed * windVector;

	texCoord3.st = texCoord0.pq * 160.0 + frame * 2.5;
	texCoord3.pq = texCoord0.pq *  90.0 - frame * 2.0;
	texCoord4.st = texCoord0.pq *   2.0;
	texCoord4.pq = texCoord0.pq *   6.0 + frame * 0.37;


	// SIMULATE WAVES
	vertexPos.y = 3.0 * (cos(frame * 500.0 + vertexPosAttr.z) * sin(frame * 500.0 + vertexPosAttr.x / 1000.0));

	// COMPUTE LIGHT VECTORS
	eyeVec = eyePos - vertexPos.xyz;
	sunVec = normalize(SunDir * 20000.0 + MapMid - vertexPos.xyz);

	// FOG eye-depth
	worldPos = vertexPos.xyz;
	fogCoord = (viewMatrix * vertexPos).z;

	// POSITION
	gl_Position = projMatrix * viewMatrix * vertexPos;
}

