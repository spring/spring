/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef __BASE_WATER_H__
#define __BASE_WATER_H__

#include "float3.h"
class CGame;

struct HeightmapChange {
	int x1,y1,x2,y2;
	HeightmapChange(const int _x1, const int _y1, const int _x2, const int _y2):
									x1(_x1), y1(_y1), x2(_x2), y2(_y2) {}
};

class CBaseWater
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

	CBaseWater(void);
	virtual ~CBaseWater(void) {}

	virtual void Draw() {}
	virtual void Update() {}
	virtual void UpdateWater(CGame* game) {}
	virtual void OcclusionQuery() {}
	virtual void HeightmapChanged(const int x1, const int y1, const int x2, const int y2) {}
	virtual void AddExplosion(const float3& pos, float strength, float size) {}
	virtual int  GetID() const { return -1; }
	virtual const char* GetName() const { return ""; }

	static void UpdateBaseWater(CGame* game);
	static CBaseWater* GetWater(CBaseWater* currWaterRenderer, int nextWaterRenderMode);
	static void PushWaterMode(int nextWaterRenderMode);
	static void PushHeightmapChange(const int x1, const int y1, const int x2, const int y2);
	static std::vector<int> waterModes;
	static std::vector<HeightmapChange> heightmapChanges;
 	static bool noWakeProjectiles;

	bool drawReflection;
	bool drawRefraction;
 	bool drawSolid;
};

extern CBaseWater* water;

#endif // __BASE_WATER_H__
