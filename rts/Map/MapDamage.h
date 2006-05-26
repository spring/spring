#ifndef __MAPDAMAGE_H__
#define __MAPDAMAGE_H__

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

#endif // __MAPDAMAGE_H__
