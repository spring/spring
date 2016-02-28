/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef GROUNDFLASH_INFO_H
#define GROUNDFLASH_INFO_H

#include "System/float3.h"
#include "System/creg/creg_cond.h"

// TODO: Handle ground flashes with more flexibility like the projectiles
struct GroundFlashInfo {
	CR_DECLARE_STRUCT(GroundFlashInfo)

	GroundFlashInfo()
		: flashSize(0.0f)
		, flashAlpha(0.0f)
		, circleGrowth(0.0f)
		, circleAlpha(0.0f)
		, ttl(0)
		, flags(0)
		, color(ZeroVector)
	{}

	float flashSize;
	float flashAlpha;
	float circleGrowth;
	float circleAlpha;
	int ttl;
	unsigned int flags;
	float3 color;
};

#endif
