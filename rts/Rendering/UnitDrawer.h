/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DRAWER_H
#define UNIT_DRAWER_H

#include <array>
#include <vector>
#include <functional>

#include "Rendering/GL/LightHandler.h"
#include "Rendering/GL/RenderDataBufferFwd.hpp"
#include "Rendering/Models/3DModel.h"
#include "Rendering/Models/ModelRenderContainer.h"
#include "Rendering/UnitDrawerState.hpp"
#include "Rendering/UnitDefImage.h"
#include "System/EventClient.h"
#include "System/type2.h"
#include "System/UnorderedMap.hpp"

struct SolidObjectDef;
struct UnitDef;
struct S3DModel;

class CSolidObject;
class CUnit;

struct Command;
struct BuildInfo;
struct SolidObjectGroundDecal;
struct IUnitDrawerState;

namespace icon {
	class CIconData;
}
namespace GL {
	struct GeometryBuffer;
}


struct GhostSolidObject {
public:
	void IncRef() {        ( refCount++     ); }
	bool DecRef() { return ((refCount--) > 1); }

public:
	SolidObjectGroundDecal* decal; //FIXME defined in legacy decal handler with a lot legacy stuff
	S3DModel* model;

	float3 pos;
	float3 dir;

	int facing; //FIXME replaced with dir-vector just legacy decal drawer uses this
	int team;
	int refCount;
	int lastDrawFrame;
};



class CUnitDrawer: public CEventClient
{
public:
	// CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return
			eventName == "RenderUnitCreated"      || eventName == "RenderUnitDestroyed"  ||
			eventName == "UnitCloaked"            || eventName == "UnitDecloaked"        ||
			eventName == "UnitEnteredRadar"       || eventName == "UnitEnteredLos"       ||
			eventName == "UnitLeftRadar"          || eventName == "UnitLeftLos"          ||
			eventName == "PlayerChanged"          || eventName == "SunChanged";
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderUnitCreated(const CUnit*, int cloaked);
	void RenderUnitDestroyed(const CUnit*);

	void UnitEnteredRadar(const CUnit* unit, int allyTeam);
	void UnitEnteredLos(const CUnit* unit, int allyTeam);
	void UnitLeftRadar(const CUnit* unit, int allyTeam);
	void UnitLeftLos(const CUnit* unit, int allyTeam);

	void UnitCloaked(const CUnit* unit);
	void UnitDecloaked(const CUnit* unit);

	void PlayerChanged(int playerNum);
	void SunChanged();

public:
	CUnitDrawer(): CEventClient("[CUnitDrawer]", 271828, false) {}

	static void InitStatic();
	static void KillStatic(bool reload);

	void Init();
	void Kill();

	void Update();

	void UpdateGhostedBuildings();

	void Draw();
	void DrawOpaquePass(bool deferredPass);
	void DrawShadowPass();
	void DrawAlphaPass(bool aboveWater);

	void SetDrawForwardPass(bool b) { drawForward = b; }
	void SetDrawDeferredPass(bool b) { drawDeferred = b; }

	static void DrawUnitModel(const CUnit* unit, bool noLuaCall);
	static void DrawUnitModelBeingBuiltShadow(const CUnit* unit, bool noLuaCall);
	static void DrawUnitModelBeingBuiltOpaque(const CUnit* unit, bool noLuaCall);

	static void SetUnitLuaTrans(const CUnit* unit, bool lodCall);
	static void SetUnitDefTrans(const CUnit* unit, bool lodCall);

	static void DrawUnitLuaTrans(const CUnit* unit, bool lodCall, bool noLuaCall, bool fullModel = false);
	static void DrawUnitDefTrans(const CUnit* unit, bool lodCall, bool noLuaCall, bool fullModel = false);

	void PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass);
	void PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass);
	void PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass);
	void PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass);

	/// LuaOpenGL::Unit{Raw}: draw a single unit with full state setup
	void PushIndividualOpaqueState(const CUnit* unit, bool deferredPass);
	void PopIndividualOpaqueState(const CUnit* unit, bool deferredPass);
	void DrawIndividualDefTrans(const CUnit* unit, bool noLuaCall);
	void DrawIndividualLuaTrans(const CUnit* unit, bool noLuaCall);

	// alpha.x := alpha-value
	// alpha.y := alpha-pass (true or false)
	void SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const;
	void SetAlphaTest(const float4& params) const;


	void SetupOpaqueDrawing(bool deferredPass);
	void ResetOpaqueDrawing(bool deferredPass);
	void SetupAlphaDrawing(bool deferredPass, bool aboveWater);
	void ResetAlphaDrawing(bool deferredPass);

	void SetupShowUnitBuildSquares(bool onMiniMap, bool testCanBuild);
	void ResetShowUnitBuildSquares(bool onMiniMap, bool testCanBuild);
	bool ShowUnitBuildSquares(const BuildInfo& buildInfo, const std::vector<Command>& commands, bool testCanBuild);

	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

public:
	typedef void (*DrawModelFunc)(const CUnit*, bool);

	const std::vector<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }

	ModelRenderContainer<CUnit>& GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	ModelRenderContainer<CUnit>& GetAlphaModelRenderer(int modelType) { return alphaModelRenderers[modelType]; }

	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }

	// always returns the default state (SSP)
	const IUnitDrawerState* GetWantedDrawerState(bool alphaPass) const {
		assert(              unitDrawerStates[DRAWER_STATE_SSP]->CanEnable(this));
		assert(!alphaPass || unitDrawerStates[DRAWER_STATE_SSP]->CanDrawAlpha());
		return unitDrawerStates[DRAWER_STATE_SSP];
	}

	IUnitDrawerState* GetDrawerState(unsigned int idx) { return (                                     unitDrawerStates[idx]); }
	IUnitDrawerState* SetDrawerState(unsigned int idx) { return (unitDrawerStates[DRAWER_STATE_SEL] = unitDrawerStates[idx]); }

	void SetUnitDefImage(const UnitDef* unitDef, const std::string& texName);
	void SetUnitDefImage(const UnitDef* unitDef, unsigned int texID, int xsize, int ysize);
	unsigned int GetUnitDefImage(const UnitDef* unitDef);

	const DrawModelFunc GetDrawModelFunc(unsigned int idx) const { return drawModelFuncs[idx]; }

	bool DrawForward() const { return drawForward; }
	bool DrawDeferred() const { return drawDeferred; }

	bool& WireFrameModeRef() { return wireFrameMode; }

public:
	struct TempDrawUnit {
		const UnitDef* unitDef;

		int team;
		int facing;
		int timeout;

		float3 pos;
		float rotation; // radians

		bool drawAlpha;
		bool drawBorder;
	};

	void AddTempDrawUnit(const TempDrawUnit& tempDrawUnit);
	void UpdateTempDrawUnits(std::vector<TempDrawUnit>& tempDrawUnits);

private:
	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const;

	bool CanDrawOpaqueUnit(const CUnit* unit, bool drawReflection, bool drawRefraction) const;
	bool CanDrawOpaqueUnitShadow(const CUnit* unit) const;

	void DrawOpaqueUnit(CUnit* unit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueUnitShadow(CUnit* unit);
	void DrawOpaqueUnitsShadow(int modelType);
	void DrawOpaqueUnits(int modelType, bool drawReflection, bool drawRefraction);

	void DrawAlphaUnits(int modelType);
	void DrawAlphaUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass);

	void DrawOpaqueAIUnits(int modelType);
	void DrawOpaqueAIUnit(const TempDrawUnit& unit);
	void DrawAlphaAIUnits(int modelType);
	void DrawAlphaAIUnit(const TempDrawUnit& unit);

	void DrawGhostedBuildings(int modelType);

public:
	void DrawUnitIcons();
	void DrawUnitMiniMapIcon(const CUnit* unit, GL::RenderDataBufferTC* buffer) const;
	void DrawUnitMiniMapIcons(GL::RenderDataBufferTC* buffer) const;

private:
	void UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed);
	void UpdateUnitIconState(CUnit* unit);

	static void DrawUnitIcon(CUnit* unit, GL::RenderDataBufferTC* buffer, bool asRadarBlip);
	static void UpdateUnitDrawPos(CUnit* unit);

public:
	static void BindModelTypeTexture(int mdlType, int texType);
	static void PushModelRenderState(int mdlType);
	static void PopModelRenderState(int mdlType);

	// never called directly; combined with PushModelRenderState(S3DModel*)
	// static void BindModelTypeTexture(const S3DModel* m) { BindModelTypeTexture(m->type, m->textureType); }
	static void PushModelRenderState(const S3DModel* m);
	static void PopModelRenderState(const S3DModel* m);

	// never called directly; combined with PushModelRenderState(CSolidObject*)
	// static void BindModelTypeTexture(const CSolidObject* o) { BindModelTypeTexture(o->model); }
	static void PushModelRenderState(const CSolidObject* o);
	static void PopModelRenderState(const CSolidObject* o);

	static void DrawObjectDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false);
	static void DrawObjectDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState, bool toScreen = false);

	/// {GuiHandler::DrawMapStuff,CursorIcons::DrawBuilds}: draw a static model without setting up any state
	void DrawStaticModelRaw(const S3DModel* mdl, const IUnitDrawerState* uds, const float3& pos, int buildFacing);


	template<typename BatchElem> void DrawStaticModelBatch(
		const std::vector<BatchElem>& batchElems,
		const std::function<const IUnitDrawerState*(const BatchElem&, const S3DModel*)>& setupStateFunc,
		const std::function<                   void(const BatchElem&, const S3DModel*)>& resetStateFunc,
		const std::function<const S3DModel*(const BatchElem&, const S3DModel*                         )>& nextModelFunc,
		const std::function<           void(const BatchElem&, const S3DModel*, const IUnitDrawerState*)>& drawModelFunc
	) {
		const S3DModel* model = nextModelFunc(batchElems.front(), nullptr);
		const IUnitDrawerState* state = setupStateFunc(batchElems.front(), model);

		for (const BatchElem& batchElem: batchElems) {
			drawModelFunc(batchElem, model = nextModelFunc(batchElem, model), state);
		}

		resetStateFunc(batchElems.back(), model);
	}

	static bool ObjectVisibleReflection(const float3 objPos, const float3 camPos, float maxRadius);


public:
	float unitDrawDist;
	float unitDrawDistSqr;
	float unitIconDist;
	float iconLength;
	float sqCamDistToGroundForIcons;

	// .x := regular unit alpha
	// .y := ghosted unit alpha (out of radar)
	// .z := ghosted unit alpha (inside radar)
	// .w := AI-temp unit alpha
	float4 alphaValues;

private:
	bool drawForward;
	bool drawDeferred;
	bool wireFrameMode;

	bool useDistToGroundForIcons;

private:
	std::array<ModelRenderContainer<CUnit>, MODELTYPE_OTHER> opaqueModelRenderers;
	std::array<ModelRenderContainer<CUnit>, MODELTYPE_OTHER> alphaModelRenderers;

	/// units being rendered (note that this is a completely
	/// unsorted set of 3DO, S3O, opaque, and cloaked models!)
	std::vector<CUnit*> unsortedUnits;

	/// AI unit ghosts
	std::array< std::vector<TempDrawUnit>, MODELTYPE_OTHER> tempOpaqueUnits;
	std::array< std::vector<TempDrawUnit>, MODELTYPE_OTHER> tempAlphaUnits;

	/// buildings that were in LOS_PREVLOS when they died and not in LOS since
	std::vector<std::array<std::vector<GhostSolidObject*>, MODELTYPE_OTHER>> deadGhostBuildings;
	/// buildings that left LOS but are still alive
	std::vector<std::array<std::vector<CUnit*>, MODELTYPE_OTHER>> liveGhostBuildings;

	/// units that are only rendered as icons this frame
	spring::unsynced_map<icon::CIconData*, std::vector<const CUnit*> > unitsByIcon;
	std::vector<icon::CIconData*> unitIcons;

	std::vector<UnitDefImage> unitDefImages;


	// caches for ShowUnitBuildSquare
	std::vector<float3> buildableSquares;
	std::vector<float3> featureSquares;
	std::vector<float3> illegalSquares;


	// [0] := no-op path
	// [1] := default shader-path
	// [2] := currently selected state
	std::array<IUnitDrawerState*, DRAWER_STATE_CNT> unitDrawerStates;
	std::array<DrawModelFunc, 3> drawModelFuncs;

private:
	GL::LightHandler lightHandler;
	GL::GeometryBuffer* geomBuffer;
};

extern CUnitDrawer* unitDrawer;

#endif // UNIT_DRAWER_H
