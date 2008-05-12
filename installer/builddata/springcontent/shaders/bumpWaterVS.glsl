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
// #define MapMid vec3
// #define SunDir vec3

uniform vec3 eyePos;
varying vec3 eyeVec;
varying vec3 ligVec;

void main(void)
{
	gl_Position = ftransform();
	gl_TexCoord[0] = gl_MultiTexCoord0;
	//gl_FrontColor  = gl_Color;
	//gl_FrontSecondaryColor = gl_SecondaryColor;
	eyeVec = eyePos - gl_Vertex.xyz;
	//ligVec = normalize(lightDir*200.0 - gl_Vertex.xyz - gl_Vertex.xyz); //FIXME use map midpoint!!
	ligVec = normalize(SunDir*20000.0 + MapMid - gl_Vertex.xyz);
}
