#ifndef __BASE_WATER_H__
#define __BASE_WATER_H__

class CGame;

class CBaseWater
{
public:
	CBaseWater(void);
	virtual ~CBaseWater(void);

	virtual void Draw()=0;
	virtual void Update(){};
	virtual void UpdateWater(CGame* game)=0;
	virtual void AddExplosion(const float3& pos, float strength, float size){};

	static CBaseWater* GetWater();

	bool drawReflection;
	bool drawRefraction;
 	bool noWakeProjectiles;
 	bool drawSolid;
};

extern CBaseWater* water;

#endif // __BASE_WATER_H__
