/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _BASE_WATER_H_
#define _BASE_WATER_H_

#include "System/float3.h"
#include "Sim/Projectiles/ExplosionListener.h"
class CGame;

class CBaseWater : public CExplosionListener
{
public:
	enum {
		WATER_RENDERER_BASIC      = 0,
		WATER_RENDERER_REFLECTIVE = 1,
		WATER_RENDERER_DYNAMIC    = 2,
		WATER_RENDERER_REFL_REFR  = 3,
		WATER_RENDERER_BUMPMAPPED = 4,
		NUM_WATER_RENDERERS       = 5,
	};

	CBaseWater();
	virtual ~CBaseWater();

	virtual void Draw() {}
	virtual void Update() {}
	virtual void UpdateWater(CGame* game) {}
	virtual void OcclusionQuery() {}
	virtual void HeightmapChanged(const int x1, const int y1, const int x2, const int y2) {}
	virtual void AddExplosion(const float3& pos, float strength, float size) {}
	virtual int  GetID() const { return -1; }
	virtual const char* GetName() const { return ""; }

	virtual void ExplosionOccurred(const CExplosionEvent& event);

	static CBaseWater* GetWater(CBaseWater* currWaterRenderer, int nextWaterRenderMode);
	static void ApplyPushedChanges(CGame* game);
	static void PushWaterMode(int nextWaterRenderMode);
	static void PushHeightmapChange(const int x1, const int y1, const int x2, const int y2);

 	static bool noWakeProjectiles;

	bool drawReflection;
	bool drawRefraction;
 	bool drawSolid;
};

extern CBaseWater* water;

#endif // _BASE_WATER_H_
