#ifndef __BASE_WATER_H__
#define __BASE_WATER_H__

#include "archdef.h"

class CGame;

class CBaseWater
{
public:
	CBaseWater(void);
	virtual ~CBaseWater(void);

	virtual void Draw()=0;
	virtual void UpdateWater(CGame* game)=0;

	static CBaseWater* GetWater();

	bool drawReflection;
};

extern CBaseWater* water;

#endif // __BASE_WATER_H__