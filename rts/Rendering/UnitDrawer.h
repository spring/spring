/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNITDRAWER_H
#define UNITDRAWER_H

#include <set>
#include <vector>
#include <list>
#include <string>
#include <map>
#include "Rendering/GL/myGL.h"
#include "System/EventClient.h"
#include "lib/gml/ThreadSafeContainers.h"

struct UAD {
	const CUnit *unit;
	int data;
	UAD(const CUnit *u, int d): unit(u), data(d) {}
	bool operator==(const UAD &u) const { return unit == u.unit && data == u.data; }
	bool operator<(const UAD &u) const { return unit < u.unit || (unit == u.unit && data < u.data); }
};

struct UnitBatch {
	static void Add(const CUnit *p);
	static void Remove(const CUnit *p);
	static void Delete(const CUnit *p) { }
};

struct CloakBatch {
	static void Add(const UAD &p);
	static void Remove(const UAD &p) { }
	static void Delete(const UAD &p) { }
};

struct LOSBatch {
	static void Add(const UAD &p);
	static void Remove(const UAD &p) { }
	static void Delete(const UAD &p) { }
};

typedef ThreadListRender<std::set<const CUnit*>, std::set<const CUnit*>, const CUnit*, UnitBatch> UnitBatchContainer;
typedef ThreadListRender<std::set<UAD>, std::set<UAD>, UAD, CloakBatch> CloakBatchContainer;
typedef ThreadListRender<std::set<UAD>, std::set<UAD>, UAD, LOSBatch> LOSBatchContainer;

class CVertexArray;
struct S3DModel;
struct UnitDef;
class CWorldObject;
class IWorldObjectModelRenderer;
class CUnit;
class CFeature;
struct Command;
struct BuildInfo;
struct BuildingGroundDecal;

namespace Shader {
	struct IProgramObject;
}

class CUnitDrawer: public CEventClient
{
public:
	CUnitDrawer();
	~CUnitDrawer(void);

	void UpdateDraw(void);
	void Update(void);
	void DeleteSynced(void);

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawCloakedUnits(bool submerged, bool noAdvShading = false);  // cloaked units must be drawn after all others
	void DrawShadowPass(void);

	void ApplyUnitTransformMatrix(CUnit*);
	void DrawUnitRaw(CUnit*);
	void DrawUnitRawModel(CUnit*);
	void DrawUnitWithLists(CUnit*, unsigned int, unsigned int);
	void DrawUnitRawWithLists(CUnit*, unsigned int, unsigned int);

	void SetTeamColour(int team, float alpha = 1.0f) const;
	void SetupForUnitDrawing(void);
	void CleanUpUnitDrawing(void) const;
	void SetupForGhostDrawing() const;
	void CleanUpGhostDrawing() const;



	bool WantsEvent(const std::string& eventName) {
		return
			(eventName == "UnitCreated"      || eventName == "UnitDestroyed") ||
			(eventName == "UnitCloaked"      || eventName == "UnitDecloaked") ||
			(eventName == "UnitEnteredLos"   || eventName == "UnitLeftLos"  ) ||
			(eventName == "UnitEnteredRadar" || eventName == "UnitLeftRadar");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void UnitCreated(const CUnit*, const CUnit*);
	void UnitDestroyed(const CUnit*, const CUnit*);
	void UnitCloaked(const CUnit*);
	void UnitDecloaked(const CUnit*);
	void UnitEnteredLos(const CUnit*, int);
	void UnitLeftLos(const CUnit*, int);
	void UnitEnteredRadar(const CUnit*, int);
	void UnitLeftRadar(const CUnit*, int);

	void UnitCreatedNow(const CUnit*);
	void UnitDestroyedNow(const CUnit*);

	void UnitLOSChange(const UAD& ua);
	void UnitCloakChange(const UAD& ua);

	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

	int ShowUnitBuildSquare(const BuildInfo&);
	int ShowUnitBuildSquare(const BuildInfo&, const std::vector<Command>&);


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
	float unitShadowDensity;

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

	struct GhostBuilding {
		BuildingGroundDecal* decal;
		float3 pos;
		S3DModel* model;
		int facing;
		int team;
	};

	bool showHealthBars;

	float3 camNorm; // used to draw far-textures

	void CreateSpecularFace(unsigned int gltype, int size, float3 baseDir, float3 xdif, float3 ydif, float3 sundir, float exponent,float3 suncolor);

	void DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing=0);
	void DrawUnitDef(const UnitDef* unitDef, int team);

	void UnitDrawingTexturesOff();
	void UnitDrawingTexturesOn();

	/** CGame::DrawDirectControlHud,  **/
	void DrawIndividual(CUnit* unit);

	const std::set<CUnit*>& GetUnsortedUnits() const { return unsortedUnits; }
	IWorldObjectModelRenderer* GetOpaqueModelRenderer(int modelType) { return opaqueModelRenderers[modelType]; }
	IWorldObjectModelRenderer* GetCloakedModelRenderer(int modelType) { return cloakedModelRenderers[modelType]; }

#ifdef USE_GML
	int multiThreadDrawUnit;
	int multiThreadDrawUnitShadow;

	volatile bool mt_drawReflection;
	volatile bool mt_drawRefraction;
	const CUnit* volatile mt_excludeUnit;

	static void DrawOpaqueUnitMT(void* c, CUnit* unit) {
		CUnitDrawer* const ud = (CUnitDrawer*) c;
		ud->DrawOpaqueUnit(unit, ud->mt_excludeUnit, ud->mt_drawReflection, ud->mt_drawRefraction);
	}

	static void DrawOpaqueUnitShadowMT(void* c, CUnit* unit) {
		((CUnitDrawer*) c)->DrawOpaqueUnitShadow(unit);
	}
#endif

private:
	bool LoadModelShaders();

	bool DrawUnitLOD(CUnit*);
	void DrawOpaqueUnit(CUnit*, const CUnit*, bool, bool);
	void DrawOpaqueUnitShadow(CUnit*);

	void DrawOpaqueUnits(int, const CUnit*, bool, bool);
	void DrawOpaqueShaderUnits();
	void DrawCloakedShaderUnits();
	void DrawShadowShaderUnits();

	void DrawOpaqueAIUnits();
	void DrawCloakedAIUnits();
	void DrawGhostedBuildings(int);
	void DrawFarTextures();
	void DrawUnitIcons(bool);

	// note: make these static?
	inline void DrawUnitDebug(CUnit*);
	void DrawUnitBeingBuilt(CUnit*);
	inline void DrawUnitModel(CUnit*);
	void DrawUnitNow(CUnit*);
	void DrawUnitStats(CUnit*);
	void UpdateDrawPos(CUnit*);

	void SetBasicTeamColour(int team, float alpha = 1.0f) const;
	void SetupBasicS3OTexture0(void) const;
	void SetupBasicS3OTexture1(void) const;
	void CleanupBasicS3OTexture1(void) const;
	void CleanupBasicS3OTexture0(void) const;
	void DrawIcon(CUnit* unit, bool asRadarBlip);
	void DrawCloakedUnitsHelper(int);
	void DrawCloakedUnitsSet(const std::set<CUnit*>&, int, bool);

	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit& unit, const float sqUnitCamDist) const;
	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;

	float cloakAlpha;
	float cloakAlpha1;
	float cloakAlpha2;
	float cloakAlpha3;

	CVertexArray* va;

	enum ModelShaderProgram {
		MODEL_SHADER_S3O_SHADOW,   // S3O model shader (V+F) with self-shadowing
		MODEL_SHADER_S3O_BASIC,    // S3O model shader (V+F) without self-shadowing
		MODEL_SHADER_S3O_ACTIVE,   // currently active S3O shader
		MODEL_SHADER_S3O_LAST
	};

	std::vector<Shader::IProgramObject*> modelShaders;
	std::vector<IWorldObjectModelRenderer*> opaqueModelRenderers;
	std::vector<IWorldObjectModelRenderer*> cloakedModelRenderers;

	// units being rendered (note that this is a completely
	// unsorted set of 3DO, S3O, opaque, and cloaked models!)
	std::set<CUnit*> unsortedUnits;

	#ifdef USE_GML
	// gmlClientServer::Work only accepts lists
	std::list<CUnit*> unsortedUnitsGML;
	#endif

	// holds S3O features and S3O projectiles
	GML_CLASSVECTOR<GML_VECTOR<CWorldObject*> > quedS3Os;
	std::set<int> usedS3OTextures;


	// buildings that were in LOS_PREVLOS when they died and not in LOS since
	std::vector<std::set<GhostBuilding*> > deadGhostBuildings;
	// buildings that left LOS but are still alive
	std::vector<std::set<CUnit*> > liveGhostBuildings;

	GML_VECTOR<CUnit*> drawFar;
	GML_VECTOR<CUnit*> drawStat;
	GML_VECTOR<CUnit*> drawIcon;

	std::vector<std::set<CUnit*> > unitRadarIcons;

	UnitBatchContainer unitBatch;
	CloakBatchContainer cloakBatch;
	LOSBatchContainer losBatch;
};

extern CUnitDrawer* unitDrawer;

#endif
