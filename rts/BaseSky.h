// BaseSky.h
///////////////////////////////////////////////////////////////////////////

#ifndef __BASE_SKY_H__
#define __BASE_SKY_H__

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
};

extern CBaseSky* sky;

#endif // __BASE_SKY_H__
