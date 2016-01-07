/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SUN_LIGHTING_H_
#define _SUN_LIGHTING_H_

#include "System/float3.h"
#include "System/float4.h"

struct CSunLighting {
public:
	void Init();

public:
	float3 groundAmbientColor;
	float3 groundSunColor;
	float3 groundSpecularColor;
	float4 unitAmbientColor;
	float4 unitSunColor;
	float3 unitSpecularColor;
	float  specularExponent;
};

extern CSunLighting sunLightingInst;

#define sunLighting (&sunLightingInst)
#endif
