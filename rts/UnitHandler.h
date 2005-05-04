// UnitHandler.h: interface for the CUnitHandler class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_UNITHANDLER_H__F18EB58F_9B37_4366_839D_9ED34468C72B__INCLUDED_)
#define AFX_UNITHANDLER_H__F18EB58F_9B37_4366_839D_9ED34468C72B__INCLUDED_

#pragma warning(disable:4786)

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CUnit;
#include <set>
#include <vector>
#include <list>
#include <stack>
#include <string>
#include <map>
class CVertexArray;
struct UnitDef;
class CBuilderCAI;
class CFeature;
class CLoadSaveInterface;
const int MAX_UNITS=5000;

class CUnitHandler  
{
public:
	int CreateChecksum();
	void Draw(bool drawReflection);
	void Update();
	void DeleteUnit(CUnit* unit);
	int AddUnit(CUnit* unit);
	CUnitHandler();
	virtual ~CUnitHandler();
	void PushNewWind(float x, float z, float strength);

	//return values for the following is
	//0 blocked
	//1 mobile unit in the way
	//2 free (or if feature is != 0 then with a blocking feature that can be reclaimed)
	int  TestUnitBuildSquare(const float3& pos, UnitDef *unitdef,CFeature *&feature);	//test if a unit can be built at specified position
	int  TestUnitBuildSquare(const float3& pos, std::string unit,CFeature *&feature);	//test if a unit can be built at specified position
	int  ShowUnitBuildSquare(const float3& pos, UnitDef *unitdef);	//test if a unit can be built at specified position and show on the ground where it's to rough
	int  TestBuildSquare(const float3& pos, UnitDef *unitdef,CFeature *&feature);	//test a singel mapsquare for build possibility

	void AddBuilderCAI(CBuilderCAI*);
	void RemoveBuilderCAI(CBuilderCAI*);
	float GetBuildHeight(float3 pos, UnitDef* unitdef);

	void DrawCloakedUnits(void);		//cloaked units must be drawn after all others;
	void DrawShadowPass(void);
	void SetupForUnitDrawing(void);
	void CleanUpUnitDrawing(void);

	void LoadSaveUnits(CLoadSaveInterface* file, bool loading);

	std::list<CUnit*> activeUnits;				//used to get all active units
	std::deque<int> freeIDs;
	CUnit* units[MAX_UNITS];							//used to get units from IDs (0 if not created)
	int overrideId;

	std::stack<CUnit*> toBeRemoved;			//units that will be removed at start of next update

	std::list<CUnit*>::iterator slowUpdateIterator;
	float unitDrawDist;

	unsigned int whiteTex;
	unsigned int radarBlippTex;

	std::set<CBuilderCAI*> builderCAIs;
	std::vector<CUnit*> drawCloaked;
	CVertexArray* va;
	inline void DrawFar(CUnit* unit);
	unsigned int unitVP;
	unsigned int unitShadowVP;

	float3 unitAmbientColor;
	float3 unitSunColor;
	float unitShadowDensity;

	int maxUnits;			//max units per team

	struct TempDrawUnit{
		UnitDef* unitdef;
		int team;
		float3 pos;
		float rot;
		bool drawBorder;
	};
	std::multimap<int,TempDrawUnit> tempDrawUnits;
	std::multimap<int,TempDrawUnit> tempTransperentDrawUnits;

	float3 camNorm;		//used by drawfar

	int lastDamageWarning;
#ifdef DIRECT_CONTROL_ALLOWED
	CUnit* playerControlledUnit;		
#endif
	bool CanCloseYard(CUnit* unit);
};

extern CUnitHandler* uh;

#endif // !defined(AFX_UNITHANDLER_H__F18EB58F_9B37_4366_839D_9ED34468C72B__INCLUDED_)

