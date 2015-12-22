/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DRAWER_H
#define UNIT_DRAWER_H

#include <vector>
#include <string>
#include <map>

#include "Rendering/GL/myGL.h"
#include "Rendering/GL/LightHandler.h"
#include "System/EventClient.h"

struct UnitDef;
struct S3DModel;

class IWorldObjectModelRenderer;
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
	void DrawOpaquePass(const CUnit* excludeUnit, bool deferredPass, bool drawReflection, bool drawRefraction);
	void DrawShadowPass();
	void DrawAlphaPass(bool noAdvShading = false);

	void SetDrawForwardPass(bool b) { drawForward = b; }
	void SetDrawDeferredPass(bool b) { drawDeferred = b; }

	// note: make these static?
	void DrawUnitModel(const CUnit* unit, bool noLuaCall);
	void DrawUnitModelBeingBuilt(const CUnit* unit, bool noLuaCall);

	void DrawUnitNoTrans(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);
	void DrawUnit(const CUnit* unit, unsigned int preList, unsigned int postList, bool lodCall, bool noLuaCall);

	/// LuaOpenGL::Unit{Raw}: draw a single unit with full state setup
	bool DrawIndividualPreCommon(const CUnit* unit);
	void DrawIndividualPostCommon(const CUnit* unit, bool dbg);
	void DrawIndividual(const CUnit* unit, bool noLuaCall);
	void DrawIndividualNoTrans(const CUnit* unit, bool noLuaCall);

	void SetTeamColour(int team, float alpha = 1.0f) const;


	// these handle either an opaque- or an alpha-pass (depending on advShading)
	void SetupOpaqueAlphaDrawing(bool deferredPass);
	void ResetOpaqueAlphaDrawing(bool deferredPass);

	void SetupOpaqueDrawing(bool deferredPass);
	void ResetOpaqueDrawing(bool deferredPass) const;
	void SetupAlphaDrawing(bool deferredPass);
	void ResetAlphaDrawing(bool deferredPass) const;


	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo);
	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands);

	void CreateSpecularFace(unsigned int glType, int size, float3 baseDir, float3 xDif, float3 yDif, float3 sunDir, float exponent, float3 sunColor);

	static void DrawBuildingSample(const UnitDef* unitdef, int team, float3 pos, int facing = 0);
	static void DrawUnitDef(const UnitDef* unitDef, int team);

	void DrawUnitMiniMapIcons() const;

	const std::vector<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }

	IWorldObjectModelRenderer* GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	IWorldObjectModelRenderer* GetAlphaModelRenderer(int modelType) { return alphaModelRenderers[modelType]; }

	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return geomBuffer; }

	const IUnitDrawerState* GetWantedDrawerState() const;


	bool DrawForward() const { return drawForward; }
	bool DrawDeferred() const { return drawDeferred; }

	bool UseAdvShading() const { return advShading; }
	bool UseAdvFading() const { return advFading; }

	bool& UseAdvShadingRef() { return advShading; }
	bool& UseAdvFadingRef() { return advFading; }

	void SetUseAdvShading(bool b) { advShading = b; }
	void SetUseAdvFading(bool b) { advFading = b; }

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

	bool CanDrawOpaqueUnit(const CUnit* unit, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction) const;
	bool CanDrawOpaqueUnitShadow(const CUnit* unit) const;

	void DrawOpaqueUnit(CUnit* unit, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueUnitShadow(CUnit* unit);
	void DrawOpaqueUnitsShadow(int modelType);
	void DrawOpaqueUnits(int modelType, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);

	void DrawAlphaUnitsHelper(int modelType);
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
	static void BindModelTypeTexture(int modelType, int texType, bool solo = false);
	static void BindModelTypeTexture(const S3DModel* m, bool solo = false);
	static void BindModelTypeTexture(const CSolidObject* o, bool solo = false);

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

private:
	bool drawForward;
	bool drawDeferred;

	bool advShading;
	bool advFading;

	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;

	float cloakAlpha;
	float cloakAlpha1;
	float cloakAlpha2;
	float cloakAlpha3;

	std::vector<IWorldObjectModelRenderer*> opaqueModelRenderers;
	std::vector<IWorldObjectModelRenderer*> alphaModelRenderers;

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

	GL::LightHandler lightHandler;
	GL::GeometryBuffer* geomBuffer;
};

extern CUnitDrawer* unitDrawer;

#endif // UNIT_DRAWER_H
