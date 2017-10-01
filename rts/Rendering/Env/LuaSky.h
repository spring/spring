/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef LUA_SKY_H
#define LUA_SKY_H

#include "ISky.h"

class CLuaSky : public ISky
{
public:
	void Update() override {}
	void Draw() override;
	void DrawSun() override;

	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}
};

#endif

