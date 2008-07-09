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
// #define MapMid           vec3
// #define SunDir           vec3
// #define PerlinStartFreq  float
// #define PerlinLacunarity float
// #define PerlinAmp        float

#define Speed 12.0

uniform float frame;
uniform vec3 eyePos;
varying vec3 eyeVec;
varying vec3 ligVec;

void main(void)
{
	gl_Position = ftransform();

	const float fstart = PerlinStartFreq;
	const float f      = PerlinLacunarity;
	gl_TexCoord[0]     = gl_MultiTexCoord0;
	gl_TexCoord[1].st = (vec2(-1.0,-1.0)+gl_MultiTexCoord0.st+0.75)*fstart      +frame*Speed;
	gl_TexCoord[1].pq = (vec2(-1.0, 1.0)+gl_MultiTexCoord0.st+0.50)*fstart*f    -frame*Speed;
	gl_TexCoord[2].st = (vec2( 1.0,-1.0)+gl_MultiTexCoord0.st+0.25)*fstart*f*f  +frame*Speed*vec2(1.0,-1.0);
	gl_TexCoord[2].pq = (vec2( 1.0, 1.0)+gl_MultiTexCoord0.st+0.00)*fstart*f*f*f+frame*Speed*vec2(-1.0,1.0);

	eyeVec = eyePos - gl_Vertex.xyz;
	ligVec = normalize(SunDir*20000.0 + MapMid - gl_Vertex.xyz);
}
