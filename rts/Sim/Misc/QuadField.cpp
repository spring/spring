/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "QuadField.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/creg/STL_List.h"

#ifndef UNIT_TEST
	#include "Sim/Features/Feature.h"
	#include "Sim/Units/Unit.h"
	#include "Sim/Projectiles/Projectile.h"
#endif

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

	// qsx and qsz are always equal
	const float maxSqLength = (radius + quadSizeX * 0.72f) * (radius + quadSizeZ * 0.72f);

	const int maxx = std::min(int(pos.x + radius) / quadSizeX + 1, numQuadsX - 1);
	const int maxz = std::min(int(pos.z + radius) / quadSizeZ + 1, numQuadsZ - 1);

	const int minx = std::max(int(pos.x - radius) / quadSizeX, 0);
	const int minz = std::max(int(pos.z - radius) / quadSizeZ, 0);

	if (maxz < minz || maxx < minx) {
		return ret;
	}

	ret.reserve((maxz - minz) * (maxx - minx));
	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
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


std::vector<int> CQuadField::GetQuadsRectangle(const float3 pos1, const float3 pos2)
{
	pos1.AssertNaNs();
	pos2.AssertNaNs();
	std::vector<int> ret;

	const int maxx = Clamp(int(pos2.x) / quadSizeX + 1, 0, numQuadsX - 1);
	const int maxz = Clamp(int(pos2.z) / quadSizeZ + 1, 0, numQuadsZ - 1);

	const int minx = Clamp(int(pos1.x) / quadSizeX, 0, numQuadsX - 1);
	const int minz = Clamp(int(pos1.z) / quadSizeZ, 0, numQuadsZ - 1);

	if (maxz < minz || maxx < minx)
		return ret;

	ret.reserve((maxz - minz) * (maxx - minx));
	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
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
	if (dir.z == 0.f) {
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
	const auto& newQuads = GetQuads(unit->pos, unit->radius);

	// compare if the quads have changed, if not stop here
	if (newQuads.size() == unit->quads.size()) {
		if (std::equal(newQuads.begin(), newQuads.end(), unit->quads.begin())) {
			return;
		}
	}

	for (const int qi: unit->quads) {
		std::list<CUnit*>& quadUnits     = baseQuads[qi].units;
		std::list<CUnit*>& quadAllyUnits = baseQuads[qi].teamUnits[unit->allyteam];
		std::list<CUnit*>::iterator ui;

		ui = std::find(quadUnits.begin(), quadUnits.end(), unit);
		if (ui != quadUnits.end())
			quadUnits.erase(ui);

		ui = std::find(quadAllyUnits.begin(), quadAllyUnits.end(), unit);
		if (ui != quadAllyUnits.end())
			quadAllyUnits.erase(ui);
	}

	for (const int qi: newQuads) {
		baseQuads[qi].units.push_front(unit);
		baseQuads[qi].teamUnits[unit->allyteam].push_front(unit);
	}
	unit->quads = newQuads;
}

void CQuadField::RemoveUnit(CUnit* unit)
{
	for (const int qi: unit->quads) {
		std::list<CUnit*>& quadUnits     = baseQuads[qi].units;
		std::list<CUnit*>& quadAllyUnits = baseQuads[qi].teamUnits[unit->allyteam];
		std::list<CUnit*>::iterator ui;

		ui = std::find(quadUnits.begin(), quadUnits.end(), unit);
		if (ui != quadUnits.end())
			quadUnits.erase(ui);

		ui = std::find(quadAllyUnits.begin(), quadAllyUnits.end(), unit);
		if (ui != quadAllyUnits.end())
			quadAllyUnits.erase(ui);
	}
	unit->quads.clear();
}



void CQuadField::AddFeature(CFeature* feature)
{
	const auto& newQuads = GetQuads(feature->pos, feature->radius);

	for (const int qi: newQuads) {
		baseQuads[qi].features.push_front(feature);
	}
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	const auto& quads = GetQuads(feature->pos, feature->radius);

	for (const int qi: quads) {
		baseQuads[qi].features.remove(feature);
	}

	#ifdef DEBUG_QUADFIELD
	for (int x = 0; x < numQuadsX; x++) {
		for (int z = 0; z < numQuadsZ; z++) {
			const Quad& q = baseQuads[z * numQuadsX + x];
			const std::list<CFeature*>& fs = q.features;

			for (CFeature* f: fs) {
				assert(f != feature);
			}
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

	const CProjectile::QuadFieldCellData& qfcd = p->GetQuadFieldCellData();

	const int2& oldCellCoors = qfcd.GetCoor(0);
	const int2 newCellCoors = {
		Clamp(int(p->pos.x / quadSizeX), 0, numQuadsX - 1),
		Clamp(int(p->pos.z / quadSizeZ), 0, numQuadsZ - 1)
	};

	if (newCellCoors != oldCellCoors) {
		RemoveProjectile(p);
		AddProjectile(p);
	}
}

void CQuadField::AddProjectile(CProjectile* p)
{
	assert(p->synced);

	CProjectile::QuadFieldCellData qfcd;

	typedef std::list<CProjectile*> List;

	if (p->hitscan) {
		// all coordinates always map to a valid quad
		qfcd.SetCoor(0, WorldPosToQuadField(p->pos));
		qfcd.SetCoor(1, WorldPosToQuadField(p->pos + p->speed * 0.5f));
		qfcd.SetCoor(2, WorldPosToQuadField(p->pos + p->speed       ));

		Quad& cell = baseQuads[numQuadsX * qfcd.GetCoor(0).y + qfcd.GetCoor(0).x];
		List& list = cell.projectiles;

		// projectiles are point-objects so they exist
		// only in a single cell EXCEPT hit-scan types
		qfcd.SetIter(0, list.insert(list.end(), p));

		for (unsigned int n = 1; n < 3; n++) {
			Quad& ncell = baseQuads[numQuadsX * qfcd.GetCoor(n).y + qfcd.GetCoor(n).x];
			List& nlist = ncell.projectiles;

			// prevent possible double insertions (into the same quad-list)
			// if case p->speed is not large enough to reach adjacent quads
			if (qfcd.GetCoor(n) != qfcd.GetCoor(n - 1)) {
				qfcd.SetIter(n, nlist.insert(nlist.end(), p));
			} else {
				qfcd.SetIter(n, nlist.end());
			}
		}
	} else {
		qfcd.SetCoor(0,  WorldPosToQuadField(p->pos));

		Quad& cell = baseQuads[numQuadsX * qfcd.GetCoor(0).y + qfcd.GetCoor(0).x];
		List& list = cell.projectiles;

		qfcd.SetIter(0, list.insert(list.end(), p));
	}

	p->SetQuadFieldCellData(qfcd);
}

void CQuadField::RemoveProjectile(CProjectile* p)
{
	assert(p->synced);

	CProjectile::QuadFieldCellData& qfcd = p->GetQuadFieldCellData();

	typedef std::list<CProjectile*> List;

	if (p->hitscan) {
		for (unsigned int n = 0; n < 3; n++) {
			Quad& cell = baseQuads[numQuadsX * qfcd.GetCoor(n).y + qfcd.GetCoor(n).x];
			List& list = cell.projectiles;

			if (qfcd.GetIter(n) != list.end()) {
				// this is O(1) instead of O(n) and crucially
				// important for projectiles, but less clean
				list.erase(qfcd.GetIter(n));
			}

			qfcd.SetIter(n, list.end());
		}
	} else {
		Quad& cell = baseQuads[numQuadsX * qfcd.GetCoor(0).y + qfcd.GetCoor(0).x];
		List& list = cell.projectiles;

		assert(qfcd.GetIter(0) != list.end());

		list.erase(qfcd.GetIter(0));
		qfcd.SetIter(0, list.end());
	}
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
				pos.SqDistance(u->midPos):
				pos.SqDistance2D(u->midPos);

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
			const float3& pos = unit->midPos;

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
				pos.SqDistance(f->midPos):
				pos.SqDistance2D(f->midPos);

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
			const float3& pos = feature->midPos;

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
			if ((pos - u->midPos).SqLength() >= Square(radius + u->radius))
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
			if ((pos - f->midPos).SqLength() >= Square(radius + f->radius))
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
	std::vector<CFeature*>& features,
	unsigned int* numUnitsPtr,
	unsigned int* numFeaturesPtr
) {
	const int tempNum = gs->tempNum++;

	// start counting from the previous object-cache sizes
	unsigned int numUnits = (numUnitsPtr == NULL)? 0: (*numUnitsPtr);
	unsigned int numFeatures = (numFeaturesPtr == NULL)? 0: (*numFeaturesPtr);

	// bail early if caches are already full
	if (numUnits >= units.size() && numFeatures >= features.size())
		return;

	assert(numUnits == 0 || numUnits == units.size() || units[numUnits] == NULL);
	assert(numFeatures == 0 || numFeatures == features.size() || features[numFeatures] == NULL);

	const auto& quads = GetQuads(pos, radius);

	for (const int qi: quads) {
		const Quad& quad = baseQuads[qi];

		// bail early if caches are already full
		if (numUnits >= units.size() && numFeatures >= features.size())
			break;

		for (CUnit* u: quad.units) {
			// bail early if cache is full
			if (numUnits >= units.size()) //FIXME
				break;

			// prevent double adding
			if (u->tempNum == tempNum)
				continue;

			const auto* colvol = u->collisionVolume;
			const float totRad = radius + colvol->GetBoundingRadius();

			if (pos.SqDistance(colvol->GetWorldSpacePos(u)) >= (totRad * totRad))
				continue;

			assert(numUnits < units.size());

			if (numUnits < units.size()) {
				u->tempNum = tempNum;
				units[numUnits++] = u;
			}
		}

		for (CFeature* f: quad.features) {
			// bail early if cache is full
			if (numFeatures >= features.size()) //FIXME
				break;

			// prevent double adding
			if (f->tempNum == tempNum)
				continue;

			const auto* colvol = f->collisionVolume;
			const float totRad = radius + colvol->GetBoundingRadius();

			if (pos.SqDistance(colvol->GetWorldSpacePos(f)) >= (totRad * totRad))
				continue;

			assert(numFeatures < features.size());

			if (numFeatures < features.size()) {
				f->tempNum = tempNum;
				features[numFeatures++] = f;
			}
		}
	}

	assert(numUnits <= units.size());
	assert(numFeatures <= features.size());

	// set end-of-list sentinels
	if (numUnits < units.size())
		units[numUnits] = NULL;
	if (numFeatures < features.size())
		features[numFeatures] = NULL;

	if (numUnitsPtr != NULL) { *numUnitsPtr = numUnits; }
	if (numFeaturesPtr != NULL) { *numFeaturesPtr = numFeatures; }
}
#endif // UNIT_TEST
