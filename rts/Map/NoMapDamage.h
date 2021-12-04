/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _NO_MAP_DAMAGE_H
#define _NO_MAP_DAMAGE_H

#include "MapDamage.h"

/** does not perform any deformation */
class CDummyMapDamage : public IMapDamage
{
public:
	void Explosion(const float3& pos, float strength, float radius) override {}
	void RecalcArea(int x1, int x2, int y1, int y2) override {}

	void Init() override { mapHardness = 0.0f; }
	void Update() override {}

	bool Disabled() const override { return true; }
};

#endif // _NO_MAP_DAMAGE_H
