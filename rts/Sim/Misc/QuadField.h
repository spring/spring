/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QUADFIELD_H
#define QUADFIELD_H

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

class CQuadField : boost::noncopyable
{
	CR_DECLARE(CQuadField);
	CR_DECLARE_SUB(Quad);

	const static int QUAD_SIZE = 256;

public:
	CQuadField();
	~CQuadField();

	std::vector<int> GetQuadsOnRay(const float3& start, float3 dir, float length);
	std::vector<int> GetQuads(float3 pos, float radius) const;
	std::vector<int> GetQuadsRectangle(const float3& pos, const float3& pos2) const;

	// optimized functions, somewhat less userfriendly
	void GetQuads(float3 pos, float radius, int*& dst) const;
	void GetQuadsOnRay(float3 start, float3 dir, float length, int*& dst);
	void GetUnitsAndFeaturesExact(const float3& pos, float radius, CUnit**& dstUnit, CFeature**& dstFeature);

	std::vector<CUnit*> GetUnits(const float3& pos, float radius);
	std::vector<CUnit*> GetUnitsExact(const float3& pos, float radius, bool spherical = true);
	std::vector<CUnit*> GetUnitsExact(const float3& mins, const float3& maxs);

	std::vector<CFeature*> GetFeaturesExact(const float3& pos, float radius);
	std::vector<CFeature*> GetFeaturesExact(const float3& mins, const float3& maxs);
	std::vector<CSolidObject*> GetSolidsExact(const float3& pos, float radius);

	void MovedUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);
	void AddFeature(CFeature* feature);
	void RemoveFeature(CFeature* feature);

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
