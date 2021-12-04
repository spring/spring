/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULL_WATER_H
#define NULL_WATER_H

#include "IWater.h"

class CLuaWater : public IWater {
public:
	CLuaWater() {}

	void Draw() override;
	void UpdateWater(CGame* game) override;

	int GetID() const override { return WATER_RENDERER_LUA; }
	const char* GetName() const override { return "lua"; }

private:
	void DrawReflection(CGame* game);
	void DrawRefraction(CGame* game);
};

#endif

