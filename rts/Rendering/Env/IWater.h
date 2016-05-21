/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_WATER_H
#define I_WATER_H

#include "System/float3.h"
#include "Sim/Projectiles/ExplosionListener.h"
class CGame;

class IWater : public IExplosionListener
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

	IWater();
	virtual ~IWater() {}

	virtual void Draw() {}
	virtual void Update() {}
	virtual void UpdateWater(CGame* game) {}
	virtual void OcclusionQuery() {}
	virtual void AddExplosion(const float3& pos, float strength, float size) {}
	virtual int  GetID() const { return -1; }
	virtual const char* GetName() const { return ""; }

	void ExplosionOccurred(const CExplosionParams& event) override;

	bool DrawReflectionPass() const { return drawReflection; }
	bool DrawRefractionPass() const { return drawRefraction; }
	bool BlockWakeProjectiles() const { return (GetID() == WATER_RENDERER_DYNAMIC); }

	static IWater* GetWater(IWater* curRenderer, int nxtRendererMode);

	static void ApplyPushedChanges(CGame* game);
	static void PushWaterMode(int nxtRendererMode);

	static void SetModelClippingPlane(const double* planeEq);

protected:
	void DrawReflections(const double* clipPlaneEqs, bool drawGround, bool drawSky);
	void DrawRefractions(const double* clipPlaneEqs, bool drawGround, bool drawSky);

protected:
	bool drawReflection;
	bool drawRefraction;
};

extern IWater* water;

#endif // I_WATER_H
