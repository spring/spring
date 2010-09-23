/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __NO_MAP_DAMAGE_H__
#define __NO_MAP_DAMAGE_H__

#include "MapDamage.h"

/** Do no deformation. */
class CNoMapDamage : public IMapDamage
{
public:
	CNoMapDamage();
	~CNoMapDamage();

	void Explosion(const float3& pos, float strength, float radius);
	void RecalcArea(int x1, int x2, int y1, int y2);
	void Update();
};

#endif // __NO_MAP_DAMAGE_H__
