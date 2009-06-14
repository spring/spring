#ifndef UNITDRAWER_H
#define UNITDRAWER_H

#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>
#include <map>
#include "Rendering/GL/myGL.h"
#include "Rendering/GL/FBO.h"

class CVertexArray;
struct S3DModel;
struct UnitDef;
class CWorldObject;
class CUnit;
class CFeature;
struct BuildingGroundDecal;


class CUnitDrawer
{
public:
	CUnitDrawer(void);
	~CUnitDrawer(void);

	void Update(void);

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawUnit(CUnit* unit);
	void DoDrawUnit(CUnit *unit, bool drawReflection, bool drawRefraction, CUnit *excludeUnit);
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
	void SetupForUnitDrawing(void) const;
	void CleanUpUnitDrawing(void) const;
	void SetupForGhostDrawing() const;
	void CleanUpGhostDrawing() const;

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

	inline void DrawFar(CUnit* unit);

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

	GML_VECTOR<CUnit*> drawCloaked;
	GML_VECTOR<CUnit*> drawCloakedS3O;

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

	FBO unitReflectFBO;

	unsigned int unitVP;             // vertex program
	unsigned int unitFP;             // fragment program, shadows disabled
	unsigned int unitShadowFP;       // fragment program, shadows enabled
	unsigned int unitShadowGenVP;    // vertex program for shadow pass

	GLuint boxtex;
	unsigned int reflTexSize;

	GLuint specularTex;
	unsigned int specTexSize;

	float unitDrawDist;
	float unitDrawDistSqr;
	float unitIconDist;
	float iconLength;

	GLuint whiteTex;

	int updateFace;

	float3 unitAmbientColor;
	float3 unitSunColor;
	float unitShadowDensity;

	struct TempDrawUnit{
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
	std::list<GhostBuilding*> ghostBuildings;	//these are buildings that where in LOS_PREVLOS when they died and havent been in los since then
	std::list<GhostBuilding*> ghostBuildingsS3O;

	bool showHealthBars;

	float3 camNorm;		//used by drawfar

	CUnit* playerControlledUnit;
	void CreateSpecularFace(unsigned int gltype, int size, float3 baseDir, float3 xdif, float3 ydif, float3 sundir, float exponent,float3 suncolor);
	void UpdateReflectTex(void);
	void CreateReflectionFace(unsigned int gltype, float3 camdir);
	void QueS3ODraw(CWorldObject* object,int textureType);
	void DrawQuedS3O(void);

	GML_CLASSVECTOR<GML_VECTOR<CWorldObject*> > quedS3Os;
	std::set<int> usedS3OTextures;

	void DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing=0);
	void DrawUnitDef(const UnitDef* unitDef, int team);

	/** CUnit::Draw **/
	void UnitDrawingTexturesOff(S3DModel *model);
	void UnitDrawingTexturesOn(S3DModel *model);

	/** CGame::DrawDirectControlHud,  **/
	void DrawIndividual(CUnit * unit);

private:
	void SetBasicTeamColour(int team) const;
	void SetupBasicS3OTexture0(void) const;
	void SetupBasicS3OTexture1(void) const;
	void CleanupBasicS3OTexture1(void) const;
	void CleanupBasicS3OTexture0(void) const;
	void DrawIcon(CUnit* unit, bool asRadarBlip);
	void DrawCloakedUnitsHelper(GML_VECTOR<CUnit*>& units, std::list<GhostBuilding*>& ghostedBuildings, bool is_s3o);
};

extern CUnitDrawer* unitDrawer;

#endif
