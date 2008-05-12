// BaseSky.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BASE_SKY_H__
#define __BASE_SKY_H__

#include "float3.h"

class CBaseSky
{
public:
	CBaseSky(void);
	virtual ~CBaseSky(void);

	virtual void Update()=0;
	virtual void Draw()=0;
	virtual void DrawSun(void)=0;

	static CBaseSky* GetSky();

	bool dynamicSky;
	float cloudDensity;

	bool wireframe;

	float fogStart;
	float3 skyColor;
	float3 sunColor;
	float3 cloudColor;
};

extern CBaseSky* sky;

#endif // __BASE_SKY_H__
