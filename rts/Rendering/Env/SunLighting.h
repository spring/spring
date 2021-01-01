/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SUN_LIGHTING_H_
#define _SUN_LIGHTING_H_

#include "System/float3.h"
#include "System/float4.h"

struct CSunLighting {
public:
	CSunLighting();
	CSunLighting(const CSunLighting& sl);
	CSunLighting& operator = (const CSunLighting& sl);

	void Init();
	void Copy(const CSunLighting& sl);

	bool SetValue(const char* key, const float4 val);
	bool SetValue(const char* key, const float val) {
		return (SetValue(key, float4(val, 0.0f, 0.0f, 0.0f)));
	}

	bool operator == (const CSunLighting& sl) const;
	bool operator != (const CSunLighting& sl) const { return (!((*this) == sl)); }

	bool IsGlobalInstance() const;

private:
	float4* colors[6];

public:
	float4 groundAmbientColor; // RGB
	float4 groundDiffuseColor; // RGB
	float4 groundSpecularColor; // RGB

	float4 modelAmbientColor;
	float4 modelDiffuseColor;
	float4 modelSpecularColor; // RGB

	float specularExponent;
	float groundShadowDensity;
	float modelShadowDensity;
};

extern CSunLighting sunLightingInst;

#define sunLighting (&sunLightingInst)
#endif
