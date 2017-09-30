/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULL_WATER_H
#define NULL_WATER_H

#include "IWater.h"

class CNullWater : public IWater
{
public:
	CNullWater() {}

	void Draw() override {}
	void UpdateWater(CGame* game) override {}
	int GetID() const override { return WATER_RENDERER_NULL; }
	const char* GetName() const override { return "null"; }
};

#endif

