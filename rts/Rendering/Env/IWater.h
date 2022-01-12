/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef I_WATER_H
#define I_WATER_H

#include "System/float4.h"
#include "Sim/Projectiles/ExplosionListener.h"

class CGame;

class IWater : public IExplosionListener
{
public:
	enum {
		WATER_RENDERER_LUA        = 0,
		WATER_RENDERER_REFLECTIVE = 1,
		WATER_RENDERER_DYNAMIC    = 2,
		WATER_RENDERER_REFRACTIVE = 3,
		WATER_RENDERER_BUMPMAPPED = 4,
		NUM_WATER_RENDERERS       = 5,
	};

	IWater(bool wantEvents = true);
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
	bool& WireFrameModeRef() { return wireFrameMode; }

	static IWater* GetWater(IWater* curRenderer, int nxtRendererMode);

	static void ApplyPushedChanges(CGame* game);
	static void PushWaterMode(int nxtRendererMode);

	static constexpr int ClipPlaneIndex() { return 2; }

	// BumpWater defaults; use d>0 to hide shoreline cracks
	static constexpr float4 MapNullClipPlane() { return {0.0f,  0.0f, 0.0f, 0.0f}; }
	static constexpr float4 MapReflClipPlane() { return {0.0f,  1.0f, 0.0f, 5.0f}; }
	static constexpr float4 MapRefrClipPlane() { return {0.0f, -1.0f, 0.0f, 5.0f}; }

	static constexpr float4 ModelNullClipPlane() { return {0.0f,  0.0f, 0.0f, 0.0f}; }
	static constexpr float4 ModelReflClipPlane() { return {0.0f,  1.0f, 0.0f, 0.0f}; }
	static constexpr float4 ModelRefrClipPlane() { return {0.0f, -1.0f, 0.0f, 0.0f}; }

protected:
	void DrawReflections(bool drawGround, bool drawSky);
	void DrawRefractions(bool drawGround, bool drawSky);

protected:
	bool drawReflection = false;
	bool drawRefraction = false;
	bool wireFrameMode = false;
};

extern IWater* water;

#endif // I_WATER_H
