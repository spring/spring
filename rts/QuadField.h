#ifndef QUADFIELD_H
#define QUADFIELD_H
// QuadField.h: interface for the CQuadField class.
//
//////////////////////////////////////////////////////////////////////

#pragma warning(disable:4786)

#include <set>
#include <vector>
#include <list>

class CUnit;
class CWorldObject;
class CFeature;
class CSolidObject;

using namespace std;

#define QUAD_SIZE 256

class CQuadField  
{
public:
	vector<int> GetQuadsOnRay(float3 start, float3 dir,float length);
	vector<CUnit*> GetUnits(const float3& pos,float radius);
	vector<CUnit*> GetUnitsExact(const float3& pos,float radius);
	void MovedUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void AddFeature(CFeature* feature);
	void RemoveFeature(CFeature* feature);
	vector<int> GetQuads(float3 pos,float radius);
	vector<int> GetQuadsRectangle(const float3& pos,const float3& pos2);
	vector<CFeature*> GetFeaturesExact(const float3& pos,float radius);
	vector<CSolidObject*> GetSolidsExact(const float3& pos,float radius);

	struct Quad {
		float startx;
		float starty;
		list<CUnit*> units;		
		list<CUnit*> teamUnits[MAX_TEAMS];
		list<CFeature*> features;
	};

	CQuadField();
	virtual ~CQuadField();

	Quad* baseQuads;
	int numQuadsX;
	int numQuadsZ;
};

extern CQuadField* qf;

#endif /* QUADFIELD_H */
