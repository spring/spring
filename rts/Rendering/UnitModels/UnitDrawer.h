#ifndef UNITDRAWER_H
#define UNITDRAWER_H

#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>
#include <map>

class CVertexArray;
struct S3DOModel;
struct UnitDef;
class CWorldObject;
class CUnit;
struct BuildingGroundDecal;

class CUnitDrawer
{
public:
	CUnitDrawer(void);
	~CUnitDrawer(void);
	void ReconfigureShaders(void);
	
	void Update(void);
	void Draw(bool drawReflection,bool drawRefraction=false);

	void DrawCloakedUnits(void);		//cloaked units must be drawn after all others;
	void DrawShadowPass(void);
	void SetupForUnitDrawing(void);
	void CleanUpUnitDrawing(void);
	void SetupForS3ODrawing(void);
	void CleanUpS3ODrawing(void);
	void CleanUpGhostDrawing();
	void SetupForGhostDrawing();

	inline void DrawFar(CUnit* unit);

	std::vector<CUnit*> drawCloaked;
	CVertexArray* va;
	bool advShading;

	unsigned int unitVP;
	unsigned int unitShadowVP;
	unsigned int unitFP;
	unsigned int boxtex;
	unsigned int units3oVP;
	unsigned int units3oFP;
	unsigned int specularTex;

	float unitDrawDist;
	float unitIconDist;
	float iconLength;

	unsigned int whiteTex;

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
	};
	std::list<GhostBuilding*> ghostBuildings;	//these are buildings that where in LOS_PREVLOS when they died and havent been in los since then

	bool showHealthBars;

	float3 camNorm;		//used by drawfar

#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* playerControlledUnit;
#endif
	void CreateSpecularFace(unsigned int gltype, int size, float3 baseDir, float3 xdif, float3 ydif, float3 sundir, float exponent,float3 suncolor);
	void UpdateReflectTex(void);
	void CreateReflectionFace(unsigned int gltype, float3 camdir);
	void QueS3ODraw(CWorldObject* object,int textureType);
	void DrawQuedS3O(void);

	std::vector<std::vector<CWorldObject*> > quedS3Os;
	std::set<int> usedS3OTextures;

	void SetS3OTeamColour(int team);
	void DrawBuildingSample(const UnitDef* unitdef, int side, float3 pos, int facing=0);

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
};

extern CUnitDrawer* unitDrawer;

#endif
