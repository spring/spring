/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef QUAD_FIELD_H
#define QUAD_FIELD_H

#include <algorithm>
#include <array>
#include <vector>

#include "System/Misc/NonCopyable.h"
#include "System/creg/creg_cond.h"
#include "System/float3.h"
#include "System/type2.h"

class CUnit;
class CFeature;
class CProjectile;
class CSolidObject;
class CPlasmaRepulser;
struct QuadFieldQuery;

template<typename T>
class QueryVectorCache {
public:
	typedef std::pair<bool, std::vector<T>> PairType;

	std::vector<T>* ReserveVector(size_t base = 0, size_t capa = 1024) {
		const auto pred = [](const PairType& p) { return (!p.first); };
		const auto iter = std::find_if(vectors.begin() + base, vectors.end(), pred);

		if (iter != vectors.end()) {
			iter->first = true;
			iter->second.clear();
			iter->second.reserve(capa);
			return &iter->second;
		}

		assert(false);
		return nullptr;
	}

	void ReserveAll(size_t capa) {
		for (size_t i = 0; i < vectors.size(); ++i) {
			ReserveVector(i, capa);
		}
	}

	void ReleaseVector(const std::vector<T>* released) {
		if (released == nullptr)
			return;

		const auto pred = [&](const PairType& p) { return (&p.second == released); };
		const auto iter = std::find_if(vectors.begin(), vectors.end(), pred);

		if (iter == vectors.end()) {
			assert(false);
			return;
		}

		iter->first = false;
	}
	void ReleaseAll() {
		for (auto& pair: vectors) {
			ReleaseVector(&pair.second);
		}
	}
private:
	// There should at most be 2 concurrent users of each vector type
	// using 3 to be safe, increase this number if the assertions below
	// fail
	std::array<PairType, 3> vectors = {{{false, {}}, {false, {}}, {false, {}}}};
};



class CQuadField : spring::noncopyable
{
	CR_DECLARE_STRUCT(CQuadField)
	CR_DECLARE_SUB(Quad)

public:

	/*
	needed to support dynamic resizing (not used yet)
	in large games the average loading factor (number of objects per quad)
	can grow too large to maintain amortized constant performance so more
	quads are needed

	static void Resize(int quadSize);
	*/

	void Init(int2 mapDims, int quadSize);
	void Kill();

	void GetQuads(QuadFieldQuery& qfq, float3 pos, float radius);
	void GetQuadsRectangle(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);
	void GetQuadsOnRay(QuadFieldQuery& qfq, const float3& start, const float3& dir, float length);

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
	void GetUnits(QuadFieldQuery& qfq, const float3& pos, float radius);
	/**
	 * Returns all units within @c radius of @c pos,
	 * takes the 3D model radius of each unit into account,
 	 * and performs the search within a sphere or cylinder depending on @c spherical
	 */
	void GetUnitsExact(QuadFieldQuery& qfq, const float3& pos, float radius, bool spherical = true);
	/**
	 * Returns all units within the rectangle defined by
	 * mins and maxs, which extends infinitely along the y-axis
	 */
	void GetUnitsExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);
	/**
	 * Returns all features within @c radius of @c pos,
	 * takes the 3D model radius of each feature into account,
	 * and performs the search within a sphere or cylinder depending on @c spherical
	 */
	void GetFeaturesExact(QuadFieldQuery& qfq, const float3& pos, float radius, bool spherical = true);
	/**
	 * Returns all features within the rectangle defined by
	 * mins and maxs, which extends infinitely along the y-axis
	 */
	void GetFeaturesExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);

	void GetProjectilesExact(QuadFieldQuery& qfq, const float3& pos, float radius);
	void GetProjectilesExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs);

	void GetSolidsExact(
		QuadFieldQuery& qfq,
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

	void ReleaseVector(std::vector<CUnit*>* v       ) { tempUnits.ReleaseVector(v); }
	void ReleaseVector(std::vector<CFeature*>* v    ) { tempFeatures.ReleaseVector(v); }
	void ReleaseVector(std::vector<CProjectile*>* v ) { tempProjectiles.ReleaseVector(v); }
	void ReleaseVector(std::vector<CSolidObject*>* v) { tempSolids.ReleaseVector(v); }
	void ReleaseVector(std::vector<int>* v          ) { tempQuads.ReleaseVector(v); }

	struct Quad {
	public:
		CR_DECLARE_STRUCT(Quad)

		Quad() = default;
		Quad(const Quad& q) = delete;
		Quad(Quad&& q) { *this = std::move(q); }

		Quad& operator = (const Quad& q) = delete;
		Quad& operator = (Quad&& q) {
			units = std::move(q.units);
			teamUnits = std::move(q.teamUnits);
			features = std::move(q.features);
			projectiles = std::move(q.projectiles);
			repulsers = std::move(q.repulsers);
			return *this;
		}

		void PostLoad();
		void Resize(int numAllyTeams) { teamUnits.resize(numAllyTeams); }
		void Clear() {
			units.clear();
			// reuse inner vectors when reloading
			// teamUnits.clear();
			for (auto& v: teamUnits) {
				v.clear();
			}
			features.clear();
			projectiles.clear();
			repulsers.clear();
		}

	public:
		std::vector<CUnit*> units;
		std::vector< std::vector<CUnit*> > teamUnits;
		std::vector<CFeature*> features;
		std::vector<CProjectile*> projectiles;
		std::vector<CPlasmaRepulser*> repulsers;
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

	constexpr static unsigned int BASE_QUAD_SIZE = 128;

private:
	int2 WorldPosToQuadField(const float3 p) const;
	int WorldPosToQuadFieldIdx(const float3 p) const;

private:
	std::vector<Quad> baseQuads;

	// preallocated vectors for Get*Exact functions
	QueryVectorCache<CUnit*> tempUnits;
	QueryVectorCache<CFeature*> tempFeatures;
	QueryVectorCache<CProjectile*> tempProjectiles;
	QueryVectorCache<CSolidObject*> tempSolids;
	QueryVectorCache<int> tempQuads;

	int numQuadsX;
	int numQuadsZ;

	int quadSizeX;
	int quadSizeZ;
};

extern CQuadField quadField;


struct QuadFieldQuery {
	~QuadFieldQuery() {
		quadField.ReleaseVector(units);
		quadField.ReleaseVector(features);
		quadField.ReleaseVector(projectiles);
		quadField.ReleaseVector(solids);
		quadField.ReleaseVector(quads);
	}

	std::vector<CUnit*>* units = nullptr;
	std::vector<CFeature*>* features = nullptr;
	std::vector<CProjectile*>* projectiles = nullptr;
	std::vector<CSolidObject*>* solids = nullptr;
	std::vector<int>* quads = nullptr;
};


#endif /* QUAD_FIELD_H */
