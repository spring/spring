/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QUAD_FIELD_H
#define QUAD_FIELD_H

#include <vector>
#include <boost/noncopyable.hpp>

#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/type2.h"

class CUnit;
class CFeature;
class CProjectile;
class CSolidObject;
class CPlasmaRepulser;


class CQuadField : boost::noncopyable
{
	CR_DECLARE_STRUCT(CQuadField)
	CR_DECLARE_SUB(Quad)

public:

/*
needed to support dynamic resizing (not used yet)
      in large games the average loading factor (number of objects per quad)
      can grow too large to maintain amortized constant performance so more
      quads are needed
*/
//	static void Resize(int quad_size);

	CQuadField(int2 mapDims, int quad_size);
	~CQuadField();

	const std::vector<int>& GetQuads(float3 pos, float radius);
	const std::vector<int>& GetQuadsRectangle(const float3& mins, const float3& maxs);
	const std::vector<int>& GetQuadsOnRay(const float3& start, const float3& dir, float length);

	void GetUnitsAndFeaturesColVol(
		const float3& pos,
		const float radius,
		std::vector<CUnit*>& units,
		std::vector<CFeature*>& features,
		std::vector<CPlasmaRepulser*>* repulsers = nullptr
	);

	/**
	 * Returns all units within @c radius of @c pos,
	 * and treats each unit as a 3D point object
	 */
	const std::vector<CUnit*>& GetUnits(const float3& pos, float radius);
	/**
	 * Returns all units within @c radius of @c pos,
	 * takes the 3D model radius of each unit into account,
 	 * and performs the search within a sphere or cylinder depending on @c spherical
	 */
	const std::vector<CUnit*>& GetUnitsExact(const float3& pos, float radius, bool spherical = true);
	/**
	 * Returns all units within the rectangle defined by
	 * mins and maxs, which extends infinitely along the y-axis
	 */
	const std::vector<CUnit*>& GetUnitsExact(const float3& mins, const float3& maxs);
	/**
	 * Returns all features within @c radius of @c pos,
	 * takes the 3D model radius of each feature into account,
	 * and performs the search within a sphere or cylinder depending on @c spherical
	 */
	const std::vector<CFeature*>& GetFeaturesExact(const float3& pos, float radius, bool spherical = true);
	/**
	 * Returns all features within the rectangle defined by
	 * mins and maxs, which extends infinitely along the y-axis
	 */
	const std::vector<CFeature*>& GetFeaturesExact(const float3& mins, const float3& maxs);

	const std::vector<CProjectile*>& GetProjectilesExact(const float3& pos, float radius);
	const std::vector<CProjectile*>& GetProjectilesExact(const float3& mins, const float3& maxs);

	const std::vector<CSolidObject*>& GetSolidsExact(
		const float3& pos,
		const float radius,
		const unsigned int physicalStateBits = 0xFFFFFFFF,
		const unsigned int collisionStateBits = 0xFFFFFFFF
	);

	bool NoSolidsExact(
		const float3& pos,
		const float radius,
		const unsigned int physicalStateBits = 0xFFFFFFFF,
		const unsigned int collisionStateBits = 0xFFFFFFFF
	);

	void MovedUnit(CUnit* unit);
	void RemoveUnit(CUnit* unit);

	void AddFeature(CFeature* feature);
	void RemoveFeature(CFeature* feature);

	void MovedProjectile(CProjectile* projectile);
	void AddProjectile(CProjectile* projectile);
	void RemoveProjectile(CProjectile* projectile);

	void MovedRepulser(CPlasmaRepulser* repulser);
	void RemoveRepulser(CPlasmaRepulser* repulser);

	struct Quad {
		CR_DECLARE_STRUCT(Quad)
		Quad();
		std::vector<CUnit*> units;
		std::vector< std::vector<CUnit*> > teamUnits;
		std::vector<CFeature*> features;
		std::vector<CProjectile*> projectiles;
		std::vector<CPlasmaRepulser*> repulsers;

		void PostLoad();
	};

	const Quad& GetQuad(unsigned i) const {
		assert(i < baseQuads.size());
		return baseQuads[i];
	}
	const Quad& GetQuadAt(unsigned x, unsigned z) const {
		assert(unsigned(numQuadsX * z + x) < baseQuads.size());
		return baseQuads[numQuadsX * z + x];
	}


	int GetNumQuadsX() const { return numQuadsX; }
	int GetNumQuadsZ() const { return numQuadsZ; }

	int GetQuadSizeX() const { return quadSizeX; }
	int GetQuadSizeZ() const { return quadSizeZ; }

	const static unsigned int BASE_QUAD_SIZE =  128;

private:
	// optimized functions, somewhat less userfriendly
	//
	// when calling these, <begQuad> and <endQuad> are both expected
	// to point to the *start* of an array of int's of size at least
	// numQuadsX * numQuadsZ (eg. tempQuads) -- GetQuadsOnRay ensures
	// this by itself, for GetQuads the callers take care of it
	//

	int2 WorldPosToQuadField(const float3 p) const;
	int WorldPosToQuadFieldIdx(const float3 p) const;

private:
	std::vector<Quad> baseQuads;

	// preallocated vectors for Get*Exact functions
	std::vector<CUnit*> tempUnits;
	std::vector<CFeature*> tempFeatures;
	std::vector<CProjectile*> tempProjectiles;
	std::vector<CSolidObject*> tempSolids;
	std::vector<int> tempQuads;

	int numQuadsX;
	int numQuadsZ;

	int quadSizeX;
	int quadSizeZ;
};

extern CQuadField* quadField;

#endif /* QUAD_FIELD_H */
