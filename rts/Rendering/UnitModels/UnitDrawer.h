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

class CVertexArray;
struct S3DModel;
struct UnitDef;
class CWorldObject;
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
	CUnitDrawer(const std::string&, int, bool);
	~CUnitDrawer(void);

	bool LoadModelShaders();
	void Update(void);

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DoDrawUnitHelper(CUnit* unit);
	void DoDrawUnit(CUnit* unit, bool drawReflection, bool drawRefraction, CUnit *excludeUnit);
	void DrawUnitLOD(CUnit* unit);

	void DrawCloakedUnits(bool submerged, bool noAdvShading = false);     // cloaked units must be drawn after all others
	void DrawShadowPass(void);
	void DoDrawUnitShadow(CUnit *unit);

	void DrawOpaqueShaderUnits();
	void DrawCloakedShaderUnits();
	void DrawShadowShaderUnits();

	void SetTeamColour(int team, float alpha = 1.0f) const;
	void SetupFor3DO() const;
	void CleanUp3DO() const;
	void SetupForUnitDrawing(void);
	void CleanUpUnitDrawing(void) const;
	void SetupForGhostDrawing() const;
	void CleanUpGhostDrawing() const;

	void SwapCloakedUnits();



	bool WantsEvent(const std::string& eventName) {
		return (eventName == "UnitCreated" || eventName == "UnitDestroyed");
	}
	bool GetFullRead() const { return true; }
	int GetReadAllyTeam() const { return AllAccessTeam; }

	void UnitCreated(const CUnit*, const CUnit*);
	void UnitDestroyed(const CUnit*, const CUnit*);


#ifdef USE_GML
	int multiThreadDrawUnit;
	int multiThreadDrawUnitShadow;

	volatile bool mt_drawReflection;
	volatile bool mt_drawRefraction;
	CUnit* volatile mt_excludeUnit;

	static void DoDrawUnitMT(void* c, CUnit* unit) {
		CUnitDrawer* const ud = (CUnitDrawer*) c;
		ud->DoDrawUnit(unit, ud->mt_drawReflection, ud->mt_drawRefraction, ud->mt_excludeUnit);
	}

	static void DoDrawUnitShadowMT(void *c,CUnit *unit) {((CUnitDrawer *)c)->DoDrawUnitShadow(unit);}
#endif

	// note: make these static?
	inline void DrawUnitDebug(CUnit*);                              // was CUnit::DrawDebug()
	void DrawUnitBeingBuilt(CUnit*);                                // was CUnit::DrawBeingBuilt()
	void ApplyUnitTransformMatrix(CUnit*);                          // was CUnit::ApplyTransformMatrix()
	inline void DrawUnitModel(CUnit*);                              // was CUnit::DrawModel()
	void DrawUnitNow(CUnit*);                                       // was CUnit::Draw()
	void DrawUnitWithLists(CUnit*, unsigned int, unsigned int);     // was CUnit::DrawWithLists() [CUnitDrawer]
	void DrawUnitRaw(CUnit*);                                       // was CUnit::DrawRaw()
	void DrawUnitRawModel(CUnit*);                                  // was CUnit::DrawRawModel() [CLuaOpenGL]
	void DrawUnitRawWithLists(CUnit*, unsigned int, unsigned int);  // was CUnit::DrawRawWithLists()
	void DrawUnitStats(CUnit*);                                     // was CUnit::DrawStats()
	void DrawUnitS3O(CUnit*);                                       // was CUnit::DrawS3O()
	void DrawFeatureStatic(CFeature*);                              // was CFeature::DrawS3O()

	void SetUnitDrawDist(float dist);
	void SetUnitIconDist(float dist);

	int ShowUnitBuildSquare(const BuildInfo&);
	int ShowUnitBuildSquare(const BuildInfo&, const std::vector<Command>&);


	GML_VECTOR<CUnit*> drawCloaked;
	GML_VECTOR<CUnit*> drawCloakedS3O;
	GML_VECTOR<CUnit*> drawCloakedSave;
	GML_VECTOR<CUnit*> drawCloakedS3OSave;

	GML_VECTOR<CUnit*> drawFar;
	GML_VECTOR<CUnit*> drawStat;

	GML_VECTOR<CUnit*> drawIcon;
	GML_VECTOR<CUnit*> drawRadarIcon;

	CVertexArray* va;

	bool advShading;
	bool advFade;
	float cloakAlpha;
	float cloakAlpha1;
	float cloakAlpha2;
	float cloakAlpha3;

	float LODScale;
	float LODScaleShadow;
	float LODScaleReflection;
	float LODScaleRefraction;

	Shader::IProgramObject* S3ODefShader;   // S3O model shader (V+F) without self-shadowing
	Shader::IProgramObject* S3OAdvShader;   // S3O model shader (V+F) with self-shadowing
	Shader::IProgramObject* S3OCurShader;   // current S3O shader (S3OShaderDef or S3OShaderAdv)

	float unitDrawDist;
	float unitDrawDistSqr;
	float unitIconDist;
	float iconLength;

	GLuint whiteTex;

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
	std::multimap<int,TempDrawUnit> tempDrawUnits;
	std::multimap<int,TempDrawUnit> tempTransparentDrawUnits;

	struct GhostBuilding {
		BuildingGroundDecal* decal;
		float3 pos;
		S3DModel* model;
		int facing;
		int team;
	};

	std::list<CUnit*> renderUnits;                 // units being rendered
	std::list<GhostBuilding*> ghostBuildings;      // buildings that where in LOS_PREVLOS when they died and not in LOS since
	std::list<GhostBuilding*> ghostBuildingsS3O;

	bool showHealthBars;

	float3 camNorm; // used to draw far-textures

	void CreateSpecularFace(unsigned int gltype, int size, float3 baseDir, float3 xdif, float3 ydif, float3 sundir, float exponent,float3 suncolor);
	void QueS3ODraw(CWorldObject* object,int textureType);
	void DrawQuedS3O(void);

	GML_CLASSVECTOR<GML_VECTOR<CWorldObject*> > quedS3Os;
	std::set<int> usedS3OTextures;

	void DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing=0);
	void DrawUnitDef(const UnitDef* unitDef, int team);

	/** CUnit::Draw **/
	void UnitDrawingTexturesOff();
	void UnitDrawingTexturesOn();

	/** CGame::DrawDirectControlHud,  **/
	void DrawIndividual(CUnit * unit);

private:
	void SetBasicTeamColour(int team, float alpha = 1.0f) const;
	void SetupBasicS3OTexture0(void) const;
	void SetupBasicS3OTexture1(void) const;
	void CleanupBasicS3OTexture1(void) const;
	void CleanupBasicS3OTexture0(void) const;
	void DrawIcon(CUnit* unit, bool asRadarBlip);
	void DrawCloakedUnitsHelper(GML_VECTOR<CUnit*>& units, std::list<GhostBuilding*>& ghostedBuildings, bool is_s3o);

	/// Returns true if the given unit should be drawn as icon in the current frame.
	bool DrawAsIcon(const CUnit& unit, const float sqUnitCamDist) const;
	bool useDistToGroundForIcons;
	float sqCamDistToGroundForIcons;
};

extern CUnitDrawer* unitDrawer;

#endif
