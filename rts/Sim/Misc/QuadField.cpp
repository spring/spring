/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "QuadField.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"

#ifndef UNIT_TEST
	#include "Sim/Features/Feature.h"
	#include "Sim/Units/Unit.h"
	#include "Sim/Projectiles/Projectile.h"
#endif

#include "System/Util.h"

CR_BIND(CQuadField, (int2(1,1), 1))
CR_REG_METADATA(CQuadField, (
	CR_MEMBER(baseQuads),
	CR_MEMBER(numQuadsX),
	CR_MEMBER(numQuadsZ),
	CR_MEMBER(quadSizeX),
	CR_MEMBER(quadSizeZ)
))

CR_BIND(CQuadField::Quad, )
CR_REG_METADATA_SUB(CQuadField, Quad, (
	CR_MEMBER(units),
	CR_MEMBER(teamUnits),
	CR_MEMBER(features),
	CR_MEMBER(projectiles)
))

CQuadField* quadField = NULL;


#ifndef UNIT_TEST
/*
void CQuadField::Resize(int quad_size)
{
	CQuadField* oldQuadField = quadField;
	CQuadField* newQuadField = new CQuadField(int2(mapDims.mapx, mapDims.mapy), quad_size);

	quadField = NULL;

	for (int zq = 0; zq < oldQuadField->GetNumQuadsZ(); zq++) {
		for (int xq = 0; xq < oldQuadField->GetNumQuadsX(); xq++) {
			const CQuadField::Quad& quad = oldQuadField->GetQuadAt(xq, zq);

			// COPY the object lists because the Remove* functions modify them
			// NOTE:
			//   teamUnits is updated internally by RemoveUnit and MovedUnit
			//
			//   if a unit exists in multiple quads in the old field, it will
			//   be removed from all of them and there is no danger of double
			//   re-insertion (important if new grid has higher resolution)
			const std::list<CUnit*      > units       = quad.units;
			const std::list<CFeature*   > features    = quad.features;
			const std::list<CProjectile*> projectiles = quad.projectiles;

			for (std::list<CUnit*>::const_iterator it = units.begin(); it != units.end(); ++it) {
				oldQuadField->RemoveUnit(*it);
				newQuadField->MovedUnit(*it); // handles addition
			}

			for (std::list<CFeature*>::const_iterator it = features.begin(); it != features.end(); ++it) {
				oldQuadField->RemoveFeature(*it);
				newQuadField->AddFeature(*it);
			}

			for (std::list<CProjectile*>::const_iterator it = projectiles.begin(); it != projectiles.end(); ++it) {
				oldQuadField->RemoveProjectile(*it);
				newQuadField->AddProjectile(*it);
			}
		}
	}

	quadField = newQuadField;

	// do this last so pointer is never dangling
	delete oldQuadField;
}
*/
#endif


CQuadField::Quad::Quad()
{
#ifndef UNIT_TEST
	teamUnits.resize(teamHandler->ActiveAllyTeams());
	assert(teamUnits.capacity() == teamHandler->ActiveAllyTeams());
#endif
}

CQuadField::CQuadField(int2 mapDims, int quad_size)
{
	quadSizeX = quad_size;
	quadSizeZ = quad_size;
	numQuadsX = (mapDims.x * SQUARE_SIZE) / quad_size;
	numQuadsZ = (mapDims.y * SQUARE_SIZE) / quad_size;

	assert(numQuadsX >= 1);
	assert(numQuadsZ >= 1);
	assert((mapDims.x * SQUARE_SIZE) % quad_size == 0);
	assert((mapDims.y * SQUARE_SIZE) % quad_size == 0);

	baseQuads.resize(numQuadsX * numQuadsZ);
}


CQuadField::~CQuadField()
{
}


int2 CQuadField::WorldPosToQuadField(const float3 p) const
{
	return int2(
		Clamp(int(p.x / quadSizeX), 0, numQuadsX - 1),
		Clamp(int(p.z / quadSizeZ), 0, numQuadsZ - 1)
	);
}


int CQuadField::WorldPosToQuadFieldIdx(const float3 p) const
{
	return Clamp(int(p.z / quadSizeZ), 0, numQuadsZ - 1) * numQuadsX + Clamp(int(p.x / quadSizeX), 0, numQuadsX - 1);
}


#ifndef UNIT_TEST
std::vector<int> CQuadField::GetQuads(float3 pos, const float radius)
{
	pos.AssertNaNs();
	pos.ClampInBounds();
	std::vector<int> ret;

	const int2 min = WorldPosToQuadField(pos - radius);
	const int2 max = WorldPosToQuadField(pos + radius);

	if (max.y < min.y || max.x < min.x)
		return ret;

	// qsx and qsz are always equal
	const float maxSqLength = (radius + quadSizeX * 0.72f) * (radius + quadSizeZ * 0.72f);

	ret.reserve((max.y - min.y) * (max.x - min.x));
	for (int z = min.y; z <= max.y; ++z) {
		for (int x = min.x; x <= max.x; ++x) {
			assert(x < numQuadsX);
			assert(z < numQuadsZ);
			const float3 quadPos = float3(x * quadSizeX + quadSizeX * 0.5f, 0, z * quadSizeZ + quadSizeZ * 0.5f);
			if (pos.SqDistance2D(quadPos) < maxSqLength) {
				ret.push_back(z * numQuadsX + x);
			}
		}
	}

	return ret;
}


std::vector<int> CQuadField::GetQuadsRectangle(const float3 mins, const float3 maxs)
{
	mins.AssertNaNs();
	maxs.AssertNaNs();
	std::vector<int> ret;

	const int2 min = WorldPosToQuadField(mins);
	const int2 max = WorldPosToQuadField(maxs);

	if (max.y < min.y || max.x < min.x)
		return ret;

	ret.reserve((max.y - min.y) * (max.x - min.x));
	for (int z = min.y; z <= max.y; ++z) {
		for (int x = min.x; x <= max.x; ++x) {
			assert(x < numQuadsX);
			assert(z < numQuadsZ);
			ret.push_back(z * numQuadsX + x);
		}
	}

	return ret;
}
#endif // UNIT_TEST


/// note: this function got an UnitTest, check the tests/ folder!
std::vector<int> CQuadField::GetQuadsOnRay(const float3 start, const float3 dir, const float length)
{
	dir.AssertNaNs();
	start.AssertNaNs();
	std::vector<int> ret;

	const float3 to = start + (dir * length);
	const float3 invQuadSize = float3(1.0f / quadSizeX, 1.0f, 1.0f / quadSizeZ);

	const bool noXdir = (math::floor(start.x * invQuadSize.x) == math::floor(to.x * invQuadSize.x));
	const bool noZdir = (math::floor(start.z * invQuadSize.z) == math::floor(to.z * invQuadSize.z));

	// often happened special case
	if (noXdir && noZdir) {
		ret.reserve(1);
		ret.push_back(WorldPosToQuadFieldIdx(start));
		assert(unsigned(ret.back()) < baseQuads.size());
		return ret;
	}

	// to prevent Div0
	if (noZdir) {
		int startX = Clamp<int>(start.x * invQuadSize.x, 0, numQuadsX - 1);
		int finalX = Clamp<int>(   to.x * invQuadSize.x, 0, numQuadsX - 1);
		if (finalX < startX) std::swap(startX, finalX);
		assert(finalX < numQuadsX);

		const int row = Clamp<int>(start.z * invQuadSize.z, 0, numQuadsZ - 1) * numQuadsX;

		ret.reserve(finalX - startX + 1);
		for (unsigned x = startX; x <= finalX; x++) {
			ret.push_back(row + x);
			assert(unsigned(ret.back()) < baseQuads.size());
		}

		return ret;
	}

	// all other
	// hint at code: we iterate the affected z-range and then for each z we compute what xs are touched
	float startZuc = start.z * invQuadSize.z;
	float finalZuc =    to.z * invQuadSize.z;
	if (finalZuc < startZuc) std::swap(startZuc, finalZuc);
	int startZ = Clamp<int>(startZuc, 0, numQuadsZ - 1);
	int finalZ = Clamp<int>(finalZuc, 0, numQuadsZ - 1);
	assert(finalZ < quadSizeZ);
	const float invDirZ = 1.0f / dir.z;

	ret.reserve(std::max(numQuadsX, numQuadsZ));
	for (int z = startZ; z <= finalZ; z++) {
		float t0 = ((z    ) * quadSizeZ - start.z) * invDirZ;
		float t1 = ((z + 1) * quadSizeZ - start.z) * invDirZ;
		if ((startZuc < 0 && z == 0) || (startZuc >= numQuadsZ && z == finalZ)) {
			t0 = ((startZuc    ) * quadSizeZ - start.z) * invDirZ;
		}
		if ((finalZuc < 0 && z == 0) || (finalZuc >= numQuadsZ && z == finalZ)) {
			t1 = ((finalZuc + 1) * quadSizeZ - start.z) * invDirZ;
		}
		t0 = Clamp(t0, 0.f, length);
		t1 = Clamp(t1, 0.f, length);

		unsigned startX = Clamp<int>((dir.x * t0 + start.x) * invQuadSize.x, 0, numQuadsX - 1);
		unsigned finalX = Clamp<int>((dir.x * t1 + start.x) * invQuadSize.x, 0, numQuadsX - 1);
		if (finalX < startX) std::swap(startX, finalX);
		assert(finalX < numQuadsX);

		const int row = Clamp(z, 0, numQuadsZ - 1) * numQuadsX;

		for (unsigned x = startX; x <= finalX; x++) {
			ret.push_back(row + x);
			assert(unsigned(ret.back()) < baseQuads.size());
		}
	}

	return ret;
}


#ifndef UNIT_TEST
void CQuadField::MovedUnit(CUnit* unit)
{
	auto newQuads = std::move(GetQuads(unit->pos, unit->radius));

	// compare if the quads have changed, if not stop here
	if (newQuads.size() == unit->quads.size()) {
		if (std::equal(newQuads.begin(), newQuads.end(), unit->quads.begin())) {
			return;
		}
	}

	for (const int qi: unit->quads) {
		std::vector<CUnit*>& quadUnits     = baseQuads[qi].units;
		std::vector<CUnit*>& quadAllyUnits = baseQuads[qi].teamUnits[unit->allyteam];

		VectorErase(quadUnits, unit);
		VectorErase(quadAllyUnits, unit);
	}

	for (const int qi: newQuads) {
		baseQuads[qi].units.push_back(unit);
		baseQuads[qi].teamUnits[unit->allyteam].push_back(unit);
	}

	unit->quads = std::move(newQuads);
}

void CQuadField::RemoveUnit(CUnit* unit)
{
	for (const int qi: unit->quads) {
		std::vector<CUnit*>& quadUnits     = baseQuads[qi].units;
		std::vector<CUnit*>& quadAllyUnits = baseQuads[qi].teamUnits[unit->allyteam];

		VectorErase(quadUnits, unit);
		VectorErase(quadAllyUnits, unit);
	}

	unit->quads.clear();
}



void CQuadField::AddFeature(CFeature* feature)
{
	const auto& newQuads = GetQuads(feature->pos, feature->radius);

	for (const int qi: newQuads) {
		baseQuads[qi].features.push_back(feature);
	}
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	const auto& quads = GetQuads(feature->pos, feature->radius);

	for (const int qi: quads) {
		VectorErase(baseQuads[qi].features, feature);
	}

	#ifdef DEBUG_QUADFIELD
	for (const Quad& q: baseQuads) {
		for (CFeature* f: q.features) {
			assert(f != feature);
		}
	}
	#endif
}



void CQuadField::MovedProjectile(CProjectile* p)
{
	if (!p->synced)
		return;
	// hit-scan projectiles do NOT move!
	if (p->hitscan)
		return;

	const int newQuad = WorldPosToQuadFieldIdx(p->pos);
	if (newQuad != p->quads.back()) {
		RemoveProjectile(p);
		AddProjectile(p);
	}
}

void CQuadField::AddProjectile(CProjectile* p)
{
	assert(p->synced);

	if (p->hitscan) {
		auto newQuads = std::move(GetQuadsOnRay(p->pos, p->dir, p->speed.w));

		for (const int qi: newQuads) {
			baseQuads[qi].projectiles.push_back(p);
		}

		p->quads = std::move(newQuads);
	} else {
		int newQuad = WorldPosToQuadFieldIdx(p->pos);
		baseQuads[newQuad].projectiles.push_back(p);
		p->quads.clear();
		p->quads.push_back(newQuad);
	}
}

void CQuadField::RemoveProjectile(CProjectile* p)
{
	assert(p->synced);

	for (const int qi: p->quads) {
		VectorErase(baseQuads[qi].projectiles, p);
	}

	p->quads.clear();
}





std::vector<CUnit*> CQuadField::GetUnits(const float3& pos, float radius)
{
	const auto& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;
	std::vector<CUnit*> units;

	for (const int qi: quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;
			units.push_back(u);
		}
	}

	return units;
}

std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& pos, float radius, bool spherical)
{
	const auto& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;
	std::vector<CUnit*> units;

	for (const int qi: quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;

			const float totRad       = radius + u->radius;
			const float totRadSq     = totRad * totRad;
			const float posUnitDstSq = spherical?
				pos.SqDistance(u->pos):
				pos.SqDistance2D(u->pos);

			if (posUnitDstSq >= totRadSq)
				continue;

			u->tempNum = tempNum;
			units.push_back(u);
		}
	}

	return units;
}

std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& mins, const float3& maxs)
{
	const auto& quads = GetQuadsRectangle(mins, maxs);
	const int tempNum = gs->tempNum++;
	std::vector<CUnit*> units;

	for (const int qi: quads) {
		for (CUnit* unit: baseQuads[qi].units) {
			const float3& pos = unit->pos;

			if (unit->tempNum == tempNum) { continue; }
			if (pos.x < mins.x || pos.x > maxs.x) { continue; }
			if (pos.z < mins.z || pos.z > maxs.z) { continue; }

			unit->tempNum = tempNum;
			units.push_back(unit);
		}
	}

	return units;
}


std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& pos, float radius, bool spherical)
{
	const auto& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;
	std::vector<CFeature*> features;

	for (const int qi: quads) {
		for (CFeature* f: baseQuads[qi].features) {
			if (f->tempNum == tempNum)
				continue;

			const float totRad       = radius + f->radius;
			const float totRadSq     = totRad * totRad;
			const float posDstSq = spherical?
				pos.SqDistance(f->pos):
				pos.SqDistance2D(f->pos);

			if (posDstSq >= totRadSq)
				continue;

			f->tempNum = tempNum;
			features.push_back(f);
		}
	}

	return features;
}

std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& mins, const float3& maxs)
{
	const auto& quads = GetQuadsRectangle(mins, maxs);
	const int tempNum = gs->tempNum++;
	std::vector<CFeature*> features;

	for (const int qi: quads) {
		for (CFeature* feature: baseQuads[qi].features) {
			const float3& pos = feature->pos;

			if (feature->tempNum == tempNum) { continue; }
			if (pos.x < mins.x || pos.x > maxs.x) { continue; }
			if (pos.z < mins.z || pos.z > maxs.z) { continue; }

			feature->tempNum = tempNum;
			features.push_back(feature);
		}
	}

	return features;
}



std::vector<CProjectile*> CQuadField::GetProjectilesExact(const float3& pos, float radius)
{
	const auto& quads = GetQuads(pos, radius);
	std::vector<CProjectile*> projectiles;

	for (const int qi: quads) {
		for (CProjectile* p: baseQuads[qi].projectiles) {
			if (pos.SqDistance(p->pos) >= Square(radius + p->radius)) {
				continue;
			}

			projectiles.push_back(p);
		}
	}

	return projectiles;
}

std::vector<CProjectile*> CQuadField::GetProjectilesExact(const float3& mins, const float3& maxs)
{
	const auto& quads = GetQuadsRectangle(mins, maxs);
	std::vector<CProjectile*> projectiles;

	for (const int qi: quads) {
		for (CProjectile* projectile: baseQuads[qi].projectiles) {
			const float3& pos = projectile->pos;

			if (pos.x < mins.x || pos.x > maxs.x) { continue; }
			if (pos.z < mins.z || pos.z > maxs.z) { continue; }

			projectiles.push_back(projectile);
		}
	}

	return projectiles;
}



std::vector<CSolidObject*> CQuadField::GetSolidsExact(
	const float3& pos,
	const float radius,
	const unsigned int physicalStateBits,
	const unsigned int collisionStateBits
) {
	const auto& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;
	std::vector<CSolidObject*> solids;

	for (const int qi: quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;
			if (!u->HasPhysicalStateBit(physicalStateBits))
				continue;
			if (!u->HasCollidableStateBit(collisionStateBits))
				continue;
			if ((pos - u->pos).SqLength() >= Square(radius + u->radius))
				continue;

			u->tempNum = tempNum;
			solids.push_back(u);
		}

		for (CFeature* f: baseQuads[qi].features) {
			if (f->tempNum == tempNum)
				continue;
			if (!f->HasPhysicalStateBit(physicalStateBits))
				continue;
			if (!f->HasCollidableStateBit(collisionStateBits))
				continue;
			if ((pos - f->pos).SqLength() >= Square(radius + f->radius))
				continue;

			f->tempNum = tempNum;
			solids.push_back(f);
		}
	}

	return solids;
}


// optimization specifically for projectile collisions
void CQuadField::GetUnitsAndFeaturesColVol(
	const float3& pos,
	const float radius,
	std::vector<CUnit*>& units,
	std::vector<CFeature*>& features
) {
	const int tempNum = gs->tempNum++;

	// start counting from the previous object-cache sizes
	const auto& quads = GetQuads(pos, radius);

	for (const int qi: quads) {
		const Quad& quad = baseQuads[qi];

		for (CUnit* u: quad.units) {

			// prevent double adding
			if (u->tempNum == tempNum)
				continue;

			const auto* colvol = &u->collisionVolume;
			const float totRad = radius + colvol->GetBoundingRadius();

			if (pos.SqDistance(colvol->GetWorldSpacePos(u)) >= (totRad * totRad))
				continue;

			u->tempNum = tempNum;
			units.push_back(u);
		}

		for (CFeature* f: quad.features) {

			// prevent double adding
			if (f->tempNum == tempNum)
				continue;

			const auto* colvol = &f->collisionVolume;
			const float totRad = radius + colvol->GetBoundingRadius();

			if (pos.SqDistance(colvol->GetWorldSpacePos(f)) >= (totRad * totRad))
				continue;

			f->tempNum = tempNum;
			features.push_back(f);
		}
	}
}
#endif // UNIT_TEST
