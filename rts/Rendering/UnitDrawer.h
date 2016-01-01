/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DRAWER_H
#define UNIT_DRAWER_H

#include <vector>
#include <string>
#include <map>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/LightHandler.h"
#include "System/EventClient.h"
#include "System/type2.h"

struct SolidObjectDef;
struct UnitDef;
struct S3DModel;

class IModelRenderContainer;
class CSolidObject;
class CUnit;

struct Command;
struct BuildInfo;
struct GhostSolidObject;
struct IUnitDrawerState;

namespace icon {
	class CIconData;
}
namespace GL {
	struct GeometryBuffer;
}

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
	void SunChanged(const float3& sunDir);

public:
	CUnitDrawer();
	~CUnitDrawer();

	void Update();

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawOpaquePass(bool deferredPass, bool drawReflection, bool drawRefraction);
	void DrawShadowPass();
	void DrawAlphaPass();

	void SetDrawForwardPass(bool b) { drawForward = b; }
	void SetDrawDeferredPass(bool b) { drawDeferred = b; }

	// note: make these static?
	void DrawUnitModel(const CUnit* unit, bool noLuaCall);
	void DrawUnitModelBeingBuilt(const CUnit* unit, bool noLuaCall);

	void DrawUnitNoTrans(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);
	void DrawUnit(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);

	void PushIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass);
	void PushIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass);
	void PopIndividualOpaqueState(const S3DModel* model, int teamID, bool deferredPass);
	void PopIndividualAlphaState(const S3DModel* model, int teamID, bool deferredPass);

	/// LuaOpenGL::Unit{Raw}: draw a single unit with full state setup
	void PushIndividualOpaqueState(const CUnit* unit, bool deferredPass);
	void PopIndividualOpaqueState(const CUnit* unit, bool deferredPass);
	void DrawIndividual(const CUnit* unit, bool noLuaCall);
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall);

	// alpha.x := alpha-value
	// alpha.y := alpha-pass (true or false)
	void SetTeamColour(int team, const float2 alpha = float2(1.0f, 0.0f)) const;


	void SetupOpaqueDrawing(bool deferredPass);
	void ResetOpaqueDrawing(bool deferredPass) const;
	void SetupAlphaDrawing(bool deferredPass);
	void ResetAlphaDrawing(bool deferredPass) const;


	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo);
	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands);

	void CreateSpecularFace(unsigned int glType, int size, float3 baseDir, float3 xDif, float3 yDif, float3 sunDir, float exponent, float3 sunColor);

	static void DrawIndividualDefOpaque(const SolidObjectDef* objectDef, int teamID, bool rawState);
	static void DrawIndividualDefAlpha(const SolidObjectDef* objectDef, int teamID, bool rawState);

	void DrawUnitMiniMapIcons() const;

	const std::vector<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }

	IModelRenderContainer* GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	IModelRenderContainer* GetAlphaModelRenderer(int modelType) { return alphaModelRenderers[modelType]; }

	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }

	const IUnitDrawerState* GetWantedDrawerState(bool alphaPass) const;


	bool DrawForward() const { return drawForward; }
	bool DrawDeferred() const { return drawDeferred; }

	bool UseAdvShading() const { return advShading; }
	bool UseAdvFading() const { return advFading; }

	bool& UseAdvShadingRef() { return advShading; }
	bool& UseAdvFadingRef() { return advFading; }

	void ResetTransform(bool b) { resetTransform = b; }

public:
	struct TempDrawUnit {
		const UnitDef* unitDef;

		int team;
		int facing;
		int timeout;

		float3 pos;
		float rotation;

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
	void DrawAlphaAIUnitBorder(const TempDrawUnit& unit);

	void DrawGhostedBuildings(int modelType);


	void DrawUnitIcons(bool drawReflection);
	void DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const;

	void UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed);
	void UpdateUnitIconState(CUnit* unit);

	static void DrawIcon(CUnit* unit, bool asRadarBlip);
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

	// needed by FFP drawer-state
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();

	static bool ObjectVisibleReflection(const CSolidObject* obj, const float3 camPos);


public:
	float unitDrawDist;
	float unitDrawDistSqr;
	float unitIconDist;
	float iconLength;

	float3 camNorm; ///< used to draw far-textures
	float3 unitAmbientColor;
	float3 unitSunColor;

	// .x := regular unit alpha
	// .y := ghosted unit alpha (out of radar)
	// .z := ghosted unit alpha (inside radar)
	// .w := AI-temp unit alpha
	float4 alphaValues;

private:
	bool drawForward;
	bool drawDeferred;

	bool advShading;
	bool advFading;

	// if true, V and P are reset at the start of a draw-pass
	bool resetTransform;

	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;

private:
	std::vector<IModelRenderContainer*> opaqueModelRenderers;
	std::vector<IModelRenderContainer*> alphaModelRenderers;

	/// units being rendered (note that this is a completely
	/// unsorted set of 3DO, S3O, opaque, and cloaked models!)
	std::vector<CUnit*> unsortedUnits;

	/// AI unit ghosts
	std::vector< std::vector<TempDrawUnit> > tempOpaqueUnits;
	std::vector< std::vector<TempDrawUnit> > tempAlphaUnits;

	/// buildings that were in LOS_PREVLOS when they died and not in LOS since
	std::vector<std::vector<GhostSolidObject*> > deadGhostBuildings;
	/// buildings that left LOS but are still alive
	std::vector<std::vector<CUnit*> > liveGhostBuildings;

	/// units that are only rendered as icons this frame
	std::vector<CUnit*> iconUnits;

	std::vector<std::vector<CUnit*> > unitRadarIcons;
	std::map<icon::CIconData*, std::vector<const CUnit*> > unitsByIcon;

	// [0] := fallback shader-less rendering path
	// [1] := default shader-driven rendering path
	// [2] := currently selected state
	std::vector<IUnitDrawerState*> unitDrawerStates;

private:
	GL::LightHandler lightHandler;
	GL::GeometryBuffer* geomBuffer;
};

extern CUnitDrawer* unitDrawer;

#endif // UNIT_DRAWER_H
