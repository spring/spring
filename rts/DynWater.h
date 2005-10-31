#ifndef DYNWATER_H
#define DYNWATER_H
#include "BaseWater.h"

class CDynWater :
	public CBaseWater
{
public:
	CDynWater(void);
	~CDynWater(void);

	virtual void Draw();
	virtual void UpdateWater(CGame* game);
	virtual void Update();

	unsigned int reflectTexture;
	unsigned int bumpTexture;
	unsigned int rawBumpTexture[4];
	float3 waterSurfaceColor;

	unsigned int waterFP;

};
#endif
