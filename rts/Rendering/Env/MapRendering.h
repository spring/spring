/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAP_RENDERING_H_
#define _MAP_RENDERING_H_

#include "System/float4.h"

struct CMapRendering {
public:
	void Init();
	bool IsGlobalInstance() const;
public:
	float4 splatTexScales;
	float4 splatTexMults;
	bool   splatDetailNormalDiffuseAlpha;
	bool   voidWater;
	bool   voidGround;
};

extern CMapRendering mapRenderingInst;

#define mapRendering (&mapRenderingInst)
#endif
