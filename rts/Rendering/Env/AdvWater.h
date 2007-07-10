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
	void Draw(bool useBlending);
	CAdvWater(bool loadShader=true);
	virtual ~CAdvWater();
	virtual int GetID() const { return 1; }

	unsigned int reflectTexture;
	unsigned int bumpTexture;
	unsigned int rawBumpTexture[4];
	float3 waterSurfaceColor;

	unsigned int waterFP;
};

#endif // __ADV_WATER_H__

