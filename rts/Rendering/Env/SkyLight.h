/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _SKY_LIGHT_H_
#define _SKY_LIGHT_H_

#include "System/float4.h"

struct ISkyLight {
public:
	ISkyLight();

	// no need to update the specular cubemap if this returns false, etc
	bool Update() {
		if (cacheDir != lightDir) {
			cacheDir = lightDir;
			return true;
		}
		return false;
	}

	void SetLightDir(const float4& dir) { lightDir = dir; }

	const float4& GetLightDir() const { return lightDir; }
	const float GetLightIntensity() const { return lightDir.w; }

protected:
	float4 lightDir;
	float4 cacheDir;
};

#endif

