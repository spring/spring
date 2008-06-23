#ifndef UNITDRAWER_H
#define UNITDRAWER_H

#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>
#include <map>
#include "Rendering/GL/myGL.h"

class CVertexArray;
struct S3DOModel;
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

  volatile bool mt_drawReflection;
  volatile bool mt_drawRefraction;
	CUnit * volatile mt_excludeUnit;

	void Draw(bool drawReflection, bool drawRefraction = false);
	void DrawUnit(CUnit* unit);
	void DoDrawUnit(CUnit *unit, bool drawReflection, bool drawRefraction, CUnit *excludeUnit);
	static void DoDrawUnitMT(void *c,CUnit *unit) {((CUnitDrawer *)c)->DoDrawUnit(unit,((CUnitDrawer *)c)->mt_drawReflection,((CUnitDrawer *)c)->mt_drawRefraction,((CUnitDrawer *)c)->mt_excludeUnit);}
	void DrawUnitLOD(CUnit* unit);

	void DrawCloakedUnits(void);									// cloaked units must be drawn after all others
	void DrawShadowPass(void);
  void DoDrawUnitShadow(CUnit *unit);
	static void DoDrawUnitShadowMT(void *c,CUnit *unit) {((CUnitDrawer *)c)->DoDrawUnitShadow(unit);}
	void SetupForUnitDrawing(void);
	void CleanUpUnitDrawing(void);
	void SetupForS3ODrawing(void);
	void CleanUpS3ODrawing(void);
	void CleanUpGhostDrawing();
	void SetupForGhostDrawing();
	void SetupForGhostDrawingS3O();

	void DrawOpaqueShaderUnits();
	void DrawCloakedShaderUnits();
	void DrawShadowShaderUnits();

	inline void DrawFar(CUnit* unit);

	// note: make these static?
	inline void DrawUnitDebug(CUnit*);								// was CUnit::DrawDebug()
	void DrawUnitBeingBuilt(CUnit*);								// was CUnit::DrawBeingBuilt()
	void ApplyUnitTransformMatrix(CUnit*);							// was CUnit::ApplyTransformMatrix()
	inline void DrawUnitModel(CUnit*);								// was CUnit::DrawModel()
	void DrawUnitNow(CUnit*);										// was CUnit::Draw()
	void DrawUnitWithLists(CUnit*, unsigned int, unsigned int);		// was CUnit::DrawWithLists() [CUnitDrawer]
	void DrawUnitRaw(CUnit*);										// was CUnit::DrawRaw()
	void DrawUnitRawModel(CUnit*);									// was CUnit::DrawRawModel() [CLuaOpenGL]
	void DrawUnitRawWithLists(CUnit*, unsigned int, unsigned int);	// was CUnit::DrawRawWithLists()
	void DrawUnitStats(CUnit*);										// was CUnit::DrawStats()
	void DrawUnitS3O(CUnit*);										// was CUnit::DrawS3O()
	void DrawFeatureS3O(CFeature*);									// was CFeature::DrawS3O()
	void DrawWorldObjectS3O(CWorldObject*);

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

	float LODScale;
	float LODScaleShadow;
	float LODScaleReflection;
	float LODScaleRefraction;

	unsigned int unitVP;             // vertex program for 3DO
	unsigned int unitFP;             // fragment program for 3DO, shadows disabled
	unsigned int unitShadowFP;       // fragment program for 3DO, shadows enabled
	unsigned int unitS3oVP;          // vertex program for S3O
	unsigned int unitS3oFP;          // fragment program for S3O, shadows disabled
	unsigned int unitShadowS3oFP;    // fragment program for S3O, shadows enabled
	unsigned int unitShadowGenVP;    // vertex program for shadow pass (both 3DO and S3O)

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
		S3DOModel* model;
		int facing;
		int team;
	};
	std::list<GhostBuilding*> ghostBuildings;	//these are buildings that where in LOS_PREVLOS when they died and havent been in los since then
	std::list<GhostBuilding*> ghostBuildingsS3O;

	bool showHealthBars;

	bool usingAtiHacks;

	float3 camNorm;		//used by drawfar

#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* playerControlledUnit;
#endif
	void CreateSpecularFace(unsigned int gltype, int size, float3 baseDir, float3 xdif, float3 ydif, float3 sundir, float exponent,float3 suncolor);
	void UpdateReflectTex(void);
	void CreateReflectionFace(unsigned int gltype, float3 camdir);
	void QueS3ODraw(CWorldObject* object,int textureType);
	void DrawQuedS3O(void);

	GML_CLASSVECTOR<GML_VECTOR<CWorldObject*> > quedS3Os;
	std::set<int> usedS3OTextures;

	void SetS3OTeamColour(int team);
	void DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing=0);
	void DrawUnitDef(const UnitDef* unitDef, int team);

	/* CUnit::Draw */
	void UnitDrawingTexturesOff(S3DOModel *model);
	void UnitDrawingTexturesOn(S3DOModel *model);

	/* CGame::DrawDirectControlHud,  */
	void DrawIndividual(CUnit * unit);

private:
	void SetBasicS3OTeamColour(int team);
	void SetupBasicS3OTexture0(void);
	void SetupBasicS3OTexture1(void);
	void CleanupBasicS3OTexture1(void);
	void CleanupBasicS3OTexture0(void);
	void DrawIcon(CUnit * unit, bool asRadarBlip);
	void DrawCloakedUnitsHelper(GML_VECTOR<CUnit*>& dC, std::list<GhostBuilding*>& gB, bool is_s3o);
};

extern CUnitDrawer* unitDrawer;

#endif
