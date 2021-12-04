/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef NULL_SKY_H
#define NULL_SKY_H

#include "ISky.h"

class CNullSky : public ISky
{
public:
	CNullSky() {}

	void Update() override {}
	void Draw(Game::DrawMode mode) override {}
	void DrawSun(Game::DrawMode mode) override {}

	void UpdateSunDir() override {}
	void UpdateSkyTexture() override {}
};

#endif

