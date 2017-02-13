/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAP_DAMAGE_H_
#define _MAP_DAMAGE_H_

#include "System/float3.h"

class IMapDamage
{
public:
	static IMapDamage* GetMapDamage();

public:
	IMapDamage();
	virtual ~IMapDamage() {}

	virtual void Explosion(const float3& pos, float strength, float radius) = 0;
	virtual void RecalcArea(int x1, int x2, int y1, int y2) = 0;
	virtual void Update() {}

	bool disabled;
	float mapHardness;
};

extern IMapDamage* mapDamage;

#endif // _MAP_DAMAGE_H_
