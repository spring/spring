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

	bool SetValue(unsigned int keyHash, const float4 val);
	bool SetValue(unsigned int keyHash, const float val) {
		return (SetValue(keyHash, float4(val, 0.0f, 0.0f, 0.0f)));
	}

	bool operator == (const CSunLighting& sl) const;
	bool operator != (const CSunLighting& sl) const { return (!((*this) == sl)); }

	bool IsGlobalInstance() const;

private:
	float4* colors[6];

public:
	float4 groundAmbientColor; // RGB
	float4 groundDiffuseColor; // RGB
	float4 groundSpecularColor; // RBG

	float4 unitAmbientColor;
	float4 unitDiffuseColor;
	float4 unitSpecularColor; // RGB

	float  specularExponent;
};

extern CSunLighting sunLightingInst;

#define sunLighting (&sunLightingInst)
#endif
