/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNIT_DRAWER_H
#define UNIT_DRAWER_H

#include <set>
#include <vector>
#include <list>
#include <string>
#include <map>
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

namespace icon {
	class CIconData;
}
namespace Shader {
	struct IProgramObject;
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

	void Draw(bool drawReflection, bool drawRefraction = false);
	/// cloaked units must be drawn after all others
	void DrawCloakedUnits(bool noAdvShading = false);
	void DrawShadowPass();

	static void DrawUnitRaw(CUnit* unit);
	static void DrawUnitRawModel(CUnit* unit);
	void DrawUnitWithLists(CUnit* unit, unsigned int preList, unsigned int postList);
	static void DrawUnitRawWithLists(CUnit* unit, unsigned int preList, unsigned int postList);

	void SetTeamColour(int team, float alpha = 1.0f) const;
	void SetupForUnitDrawing();
	void CleanUpUnitDrawing() const;
	void SetupForGhostDrawing() const;
	void CleanUpGhostDrawing() const;

	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

	bool ShowUnitBuildSquare(const BuildInfo& buildInfo);
	bool ShowUnitBuildSquare(const BuildInfo& buildInfo, const std::vector<Command>& commands);

	void CreateSpecularFace(unsigned int glType, int size, float3 baseDir, float3 xDif, float3 yDif, float3 sunDir, float exponent, float3 sunColor);

	void DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing = 0);
	static void DrawUnitDef(const UnitDef* unitDef, int team);

	void UnitDrawingTexturesOff();
	void UnitDrawingTexturesOn();

	/** CGame::DrawDirectControlHud,  **/
	void DrawIndividual(CUnit* unit);

	void DrawUnitMiniMapIcons() const;

	static unsigned int CalcUnitLOD(const CUnit* unit, unsigned int lastLOD);
	static unsigned int CalcUnitShadowLOD(const CUnit* unit, unsigned int lastLOD);
	static void SetUnitLODCount(CUnit* unit, unsigned int count);

	const std::set<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }
	IWorldObjectModelRenderer* GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	IWorldObjectModelRenderer* GetCloakedModelRenderer(int modelType) { return cloakedModelRenderers[modelType]; }

	GL::LightHandler* GetLightHandler() { return &lightHandler; }


#ifdef USE_GML
	bool multiThreadDrawUnit;
	bool multiThreadDrawUnitShadow;

	volatile bool mt_drawReflection;
	volatile bool mt_drawRefraction;
	const CUnit* volatile mt_excludeUnit;

	bool showHealthBars;

	static void DrawOpaqueUnitMT(void* c, CUnit* unit) {
		CUnitDrawer* const ud = reinterpret_cast<CUnitDrawer*>(c);
		ud->DrawOpaqueUnit(unit, ud->mt_excludeUnit, ud->mt_drawReflection, ud->mt_drawRefraction);
	}

	static void DrawOpaqueUnitShadowMT(void* c, CUnit* unit) {
		reinterpret_cast<CUnitDrawer*>(c)->DrawOpaqueUnitShadow(unit);
	}

	void DrawUnitStats(CUnit* unit);
#endif


private:
	bool LoadModelShaders();

	bool DrawUnitLOD(CUnit* unit);
	void DrawOpaqueUnit(CUnit* unit, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueUnitShadow(CUnit* unit);
	void DrawOpaqueUnitsShadow(int modelType);

	void DrawOpaqueUnits(int modelType, const CUnit* excludeUnit, bool drawReflection, bool drawRefraction);
	void DrawOpaqueShaderUnits();
	void DrawCloakedShaderUnits();
	void DrawShadowShaderUnits();

	void DrawOpaqueAIUnits();
	void DrawCloakedAIUnits();
	void DrawGhostedBuildings(int modelType);

	void DrawUnitIcons(bool drawReflection);
	void DrawUnitMiniMapIcon(const CUnit* unit, CVertexArray* va) const;
	void UpdateUnitMiniMapIcon(const CUnit* unit, bool forced, bool killed);

	// note: make these static?
	void DrawUnitBeingBuilt(CUnit* unit);
	static void DrawUnitModel(CUnit* unit);
	void DrawUnitNow(CUnit* unit);

	void UpdateUnitIconState(CUnit* unit);
	static void UpdateUnitDrawPos(CUnit* unit);

	static void SetBasicTeamColour(int team, float alpha = 1.0f);
	static void SetupBasicS3OTexture0();
	static void SetupBasicS3OTexture1();
	static void CleanupBasicS3OTexture1();
	static void CleanupBasicS3OTexture0();
	static void DrawIcon(CUnit* unit, bool asRadarBlip);
	void DrawCloakedUnitsHelper(int modelType);
	void DrawCloakedUnit(CUnit* unit, int modelType, bool drawGhostBuildingsPass);

	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit* unit, const float sqUnitCamDist) const;


public:
	bool advShading;
	bool advFade;

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
	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;

	float cloakAlpha;
	float cloakAlpha1;
	float cloakAlpha2;
	float cloakAlpha3;

	enum ModelShaderProgram {
		MODEL_SHADER_S3O_SHADOW,   ///< S3O model shader (V+F) with self-shadowing
		MODEL_SHADER_S3O_BASIC,    ///< S3O model shader (V+F) without self-shadowing
		MODEL_SHADER_S3O_ACTIVE,   ///< currently active S3O shader
		MODEL_SHADER_S3O_LAST
	};

	std::vector<Shader::IProgramObject*> modelShaders;
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
#ifdef USE_GML
	std::set<CUnit*> drawStat;
#endif

	std::vector<std::set<CUnit*> > unitRadarIcons;
	std::map<icon::CIconData*, std::set<const CUnit*> > unitsByIcon;

	GL::LightHandler lightHandler;
};

extern CUnitDrawer* unitDrawer;

#endif // UNIT_DRAWER_H
