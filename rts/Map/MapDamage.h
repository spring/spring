/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _MAPDAMAGE_H_
#define _MAPDAMAGE_H_

#include "float3.h"

class IMapDamage
{
public:
	IMapDamage(void);
	virtual ~IMapDamage(void);

	virtual void Explosion(const float3& pos, float strength,float radius) = 0;
	virtual void RecalcArea(int x1, int x2, int y1, int y2) = 0;
	virtual void Update(void) {};
	
	static IMapDamage* GetMapDamage();

	bool disabled;

	/* Ground readouts: Game/UI/MouseHandler.cpp, Game/UI/MiniMap.cpp. */
	float mapHardness; 
};

extern IMapDamage* mapDamage;

#endif // _MAPDAMAGE_H_
