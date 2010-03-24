/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __NOMAPDAMAGE_H__
#define __NOMAPDAMAGE_H__

#include "MapDamage.h"

class CNoMapDamage : public IMapDamage
{
public:
	CNoMapDamage(void);
	~CNoMapDamage(void);

	void Explosion(const float3& pos, float strength,float radius);
	void RecalcArea(int x1, int x2, int y1, int y2);
	void Update(void);
};

#endif // __NOMAPDAMAGE_H__
