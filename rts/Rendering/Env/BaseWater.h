#ifndef __BASE_WATER_H__
#define __BASE_WATER_H__

#include "float3.h"
class CGame;

class CBaseWater
{
public:
	CBaseWater(void);
	virtual ~CBaseWater(void);

	virtual void Draw() = 0;
	virtual void Update() {}
	virtual void UpdateWater(CGame* game) = 0;
	virtual void OcclusionQuery() {};
	virtual void HeightmapChanged(const int x1, const int y1, const int x2, const int y2) {}
	virtual void AddExplosion(const float3& pos, float strength, float size) {}
	virtual int  GetID() const { return -1; }

	static CBaseWater* GetWater(CBaseWater* old);

	CBaseWater* oldwater;
	void DeleteOldWater(CBaseWater *water);

	bool drawReflection;
	bool drawRefraction;
 	bool noWakeProjectiles;
 	bool drawSolid;
};

extern CBaseWater* water;

#endif // __BASE_WATER_H__
