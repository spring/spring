// DrawWater.h: interface for the CDrawWater class.
//
//////////////////////////////////////////////////////////////////////

#ifndef __ADV_WATER_H__
#define __ADV_WATER_H__

#include "BaseWater.h"

class CAdvWater : public CBaseWater
{
public:
	void UpdateWater(CGame* game);
	void Draw();
	CAdvWater();
	virtual ~CAdvWater();

	unsigned int reflectTexture;
	unsigned int bumpTexture;
	unsigned int rawBumpTexture[4];

	unsigned int waterFP;
};

#endif // __ADV_WATER_H__

