#ifndef QUADFIELD_H
#define QUADFIELD_H
// QuadField.h: interface for the CQuadField class.
//
//////////////////////////////////////////////////////////////////////

#include <set>
#include <vector>
#include <list>
#include <boost/noncopyable.hpp>

#include "creg/creg_cond.h"
#include "float3.h"

class CUnit;
class CWorldObject;
class CFeature;
class CSolidObject;

#define QUAD_SIZE 256

class CQuadField : boost::noncopyable
{
	CR_DECLARE(CQuadField);
	CR_DECLARE_SUB(Quad);

public:
	CQuadField();
	~CQuadField();

	std::vector<int> GetQuadsOnRay(const float3& start, float3 dir, float length);
	std::vector<CUnit*> GetUnits(const float3& pos, float radius);
	std::vector<CUnit*> GetUnitsExact(const float3& pos, float radius);
	std::vector<CUnit*> GetUnitsExact(const float3& mins, const float3& maxs);
	void MovedUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void AddFeature(CFeature* feature);
	void RemoveFeature(CFeature* feature);
	std::vector<int> GetQuads(float3 pos, float radius);
	std::vector<int> GetQuadsRectangle(const float3& pos, const float3& pos2);
	std::vector<CFeature*> GetFeaturesExact(const float3& pos, float radius);
	std::vector<CFeature*> GetFeaturesExact(const float3& mins, const float3& maxs);
	std::vector<CSolidObject*> GetSolidsExact(const float3& pos, float radius);

	// optimized functions, somewhat less userfriendly
	void GetQuads(float3 pos, float radius, int*& dst);
	void GetQuadsOnRay(float3 start, float3 dir, float length, int*& dst);
	void GetUnitsAndFeaturesExact(const float3& pos, float radius, CUnit**& dstUnit, CFeature**& dstFeature);

	struct Quad {
		CR_DECLARE_STRUCT(Quad);
		Quad();
		std::list<CUnit*> units;
		std::vector< std::list<CUnit*> > teamUnits;
		std::list<CFeature*> features;
	};

	const Quad& GetQuad(int i) const { assert(static_cast<unsigned>(i) < baseQuads.size()); return baseQuads[i]; }
	const Quad& GetQuadAt(int x, int z) const { return baseQuads[numQuadsX * z + x]; }
	int GetNumQuadsX() const { return numQuadsX; }
	int GetNumQuadsZ() const { return numQuadsZ; }

private:
	void Serialize(creg::ISerializer& s);

	std::vector<Quad> baseQuads;
	int numQuadsX;
	int numQuadsZ;
	int* tempQuads;
};

extern CQuadField* qf;

#endif /* QUADFIELD_H */
