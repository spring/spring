#pragma once

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
