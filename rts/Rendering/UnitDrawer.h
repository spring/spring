/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DRAWER_H
#define UNIT_DRAWER_H

#include <set>
#include <vector>
#include <list>
#include <string>
#include <map>

#include "Rendering/GL/GeometryBuffer.h"
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/LightHandler.h"
#include "System/EventClient.h"
#include "lib/gml/ThreadSafeContainers.h"

struct UnitDef;
class CWorldObject;
class IWorldObjectModelRenderer;
class CUnit;
class CFeature;
struct Command;
struct BuildInfo;
struct SolidObjectGroundDecal;
struct GhostSolidObject;
struct IUnitDrawerState;

namespace icon {
	class CIconData;
}

class CUnitDrawer: public CEventClient
{
public:
	//! CEventClient interface
	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "RenderUnitCreated"      || eventName == "RenderUnitDestroyed" ) ||
			(eventName == "RenderUnitCloakChanged" || eventName == "RenderUnitLOSChanged") ||
			(eventName == "PlayerChanged"          || eventName == "SunChanged"          );
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void RenderUnitCreated(const CUnit*, int cloaked);
	void RenderUnitDestroyed(const CUnit*);

	void RenderUnitLOSChanged(const CUnit* unit, int allyTeam, int newStatus);
	void RenderUnitCloakChanged(const CUnit* unit, int cloaked);

	void PlayerChanged(int playerNum);
	void SunChanged(const float3& sunDir);

public:
	CUnitDrawer();
	~CUnitDrawer();

	void Update();

	void SetDrawDeferredPass(bool b) {
		if ((drawDeferred = b)) {
			drawDeferred &= UpdateGeometryBuffer(false);
		}
	}

	void Draw(bool drawReflection, bool drawRefraction = false);
	/// cloaked units must be drawn after all others
	void DrawCloakedUnits(bool noAdvShading = false);
	void DrawShadowPass();
	void DrawDeferredPass(const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);

	void DrawUnitRaw(CUnit* unit);
	void DrawUnitRawModel(CUnit* unit);
	void DrawUnitWithLists(CUnit* unit, unsigned int preList, unsigned int postList);
	void DrawUnitRawWithLists(CUnit* unit, unsigned int preList, unsigned int postList);

	void SetTeamColour(int team, float alpha = 1.0f) const;
	void SetupForUnitDrawing(bool deferredPass);
	void CleanUpUnitDrawing(bool deferredPass) const;
	void SetupForGhostDrawing() const;
	void CleanUpGhostDrawing() const;

	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo);
	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands);

	void CreateSpecularFace(unsigned int glType, int size, float3 baseDir, float3 xDif, float3 yDif, float3 sunDir, float exponent, float3 sunColor);

	static void DrawBuildingSample(const UnitDef* unitdef, int team, float3 pos, int facing = 0);
	static void DrawUnitDef(const UnitDef* unitDef, int team);

	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const;

	/** LuaOpenGL::Unit{Raw} **/
	void DrawIndividual(CUnit* unit);

	void DrawUnitMiniMapIcons() const;

	static unsigned int CalcUnitLOD(const CUnit* unit, unsigned int lastLOD);
	static unsigned int CalcUnitShadowLOD(const CUnit* unit, unsigned int lastLOD);
	static void SetUnitLODCount(CUnit* unit, unsigned int count);

	const std::set<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }
	IWorldObjectModelRenderer* GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	IWorldObjectModelRenderer* GetCloakedModelRenderer(int modelType) { return cloakedModelRenderers[modelType]; }

	const GL::LightHandler* GetLightHandler() const { return &lightHandler; }
	      GL::LightHandler* GetLightHandler()       { return &lightHandler; }

	const GL::GeometryBuffer* GetGeometryBuffer() const { return &geomBuffer; }
	      GL::GeometryBuffer* GetGeometryBuffer()       { return &geomBuffer; }

	bool DrawDeferred() const { return drawDeferred; }

	bool UseAdvShading() const { return advShading; }
	bool UseAdvFading() const { return advFading; }

	bool& UseAdvShadingRef() { return advShading; }
	bool& UseAdvFadingRef() { return advFading; }

	void SetUseAdvShading(bool b) { advShading = b; }
	void SetUseAdvFading(bool b) { advFading = b; }


private:
	bool DrawUnitLOD(CUnit* unit);
	void DrawOpaqueUnit(CUnit* unit, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueUnitShadow(CUnit* unit);
	void DrawOpaqueUnitsShadow(int modelType);

	void DrawOpaqueUnits(int modelType, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueShaderUnits(unsigned int matType, bool deferredPass);
	void DrawCloakedShaderUnits(unsigned int matType);
	void DrawShadowShaderUnits(unsigned int matType);

	void DrawOpaqueAIUnits();
	void DrawCloakedAIUnits();
	void DrawGhostedBuildings(int modelType);

	void DrawUnitIcons(bool drawReflection);
	void DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const;
	void UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed);

	bool UpdateGeometryBuffer(bool init);

	// note: make these static?
	void DrawUnitBeingBuilt(CUnit* unit);
	void DrawUnitModel(CUnit* unit);
	void DrawUnitNow(CUnit* unit);

	void UpdateUnitIconState(CUnit* unit);
	static void UpdateUnitDrawPos(CUnit* unit);

	static void DrawIcon(CUnit* unit, bool asRadarBlip);
	void DrawCloakedUnitsHelper(int modelType);
	void DrawCloakedUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass);

	void SelectRenderState(bool shaderPath) {
		unitDrawerState = shaderPath? unitDrawerStateSSP: unitDrawerStateFFP;
	}

public:
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();


public:
	float LODScale;
	float LODScaleShadow;
	float LODScaleReflection;
	float LODScaleRefraction;

	float unitDrawDist;
	float unitDrawDistSqr;
	float unitIconDist;
	float iconLength;

	float3 unitAmbientColor;
	float3 unitSunColor;

	struct TempDrawUnit {
		const UnitDef* unitdef;
		int team;
		float3 pos;
		float rotation;
		int facing;
		bool drawBorder;
	};
	std::multimap<int, TempDrawUnit> tempDrawUnits;
	std::multimap<int, TempDrawUnit> tempTransparentDrawUnits;

	float3 camNorm; ///< used to draw far-textures

private:
	bool advShading;
	bool advFading;
	bool drawDeferred;

	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;

	float cloakAlpha;
	float cloakAlpha1;
	float cloakAlpha2;
	float cloakAlpha3;

	std::vector<IWorldObjectModelRenderer*> opaqueModelRenderers;
	std::vector<IWorldObjectModelRenderer*> cloakedModelRenderers;

	/**
	 * units being rendered (note that this is a completely
	 * unsorted set of 3DO, S3O, opaque, and cloaked models!)
	 */
	std::set<CUnit*> unsortedUnits;

	/// buildings that were in LOS_PREVLOS when they died and not in LOS since
	std::vector<std::set<GhostSolidObject*> > deadGhostBuildings;
	/// buildings that left LOS but are still alive
	std::vector<std::set<CUnit*> > liveGhostBuildings;

	std::set<CUnit*> drawIcon;

	std::vector<std::set<CUnit*> > unitRadarIcons;
	std::map<icon::CIconData*, std::set<const CUnit*> > unitsByIcon;

	IUnitDrawerState* unitDrawerStateSSP; // default shader-driven rendering path
	IUnitDrawerState* unitDrawerStateFFP; // fallback shader-less rendering path
	IUnitDrawerState* unitDrawerState;

	GL::LightHandler lightHandler;
	GL::GeometryBuffer geomBuffer;
};

extern CUnitDrawer* unitDrawer;

#endif // UNIT_DRAWER_H
