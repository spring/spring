/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "QuadField.h"
#include "Map/ReadMap.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "System/ContainerUtil.h"

#ifndef UNIT_TEST
	#include "Sim/Features/Feature.h"
	#include "Sim/Projectiles/Projectile.h"
	#include "Sim/Units/Unit.h"
	#include "Sim/Weapons/PlasmaRepulser.h"
#endif

CR_BIND(CQuadField, )
CR_REG_METADATA(CQuadField, (
	CR_MEMBER(baseQuads),
	CR_MEMBER(numQuadsX),
	CR_MEMBER(numQuadsZ),
	CR_MEMBER(quadSizeX),
	CR_MEMBER(quadSizeZ),
	CR_MEMBER(invQuadSize),

	CR_IGNORED(tempUnits),
	CR_IGNORED(tempFeatures),
	CR_IGNORED(tempProjectiles),
	CR_IGNORED(tempSolids),
	CR_IGNORED(tempQuads)
))

CR_BIND(CQuadField::Quad, )
CR_REG_METADATA_SUB(CQuadField, Quad, (
	CR_MEMBER(units),
	CR_IGNORED(teamUnits),
	CR_MEMBER(features),
	CR_MEMBER(projectiles),
	CR_MEMBER(repulsers),

	CR_POSTLOAD(PostLoad)
))


CQuadField quadField;


#ifndef UNIT_TEST
/*
void CQuadField::Resize(int quad_size)
{
	CQuadField* oldQuadField = &quadField;
	CQuadField newQuadField;

	newQuadField.Init(int2(mapDims.mapx, mapDims.mapy), quad_size);

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
			const std::vector<CUnit*      > units       = quad.units;
			const std::vector<CFeature*   > features    = quad.features;
			const std::vector<CProjectile*> projectiles = quad.projectiles;

			for (auto it = units.cbegin(); it != units.cend(); ++it) {
				oldQuadField->RemoveUnit(*it);
				newQuadField->MovedUnit(*it); // handles addition
			}

			for (auto it = features.cbegin(); it != features.cend(); ++it) {
				oldQuadField->RemoveFeature(*it);
				newQuadField->AddFeature(*it);
			}

			for (auto it = projectiles.cbegin(); it != projectiles.cend(); ++it) {
				oldQuadField->RemoveProjectile(*it);
				newQuadField->AddProjectile(*it);
			}
		}
	}

	quadField = std::move(newQuadField);
}
*/
#endif


void CQuadField::Quad::PostLoad()
{
#ifndef UNIT_TEST
	Resize(teamHandler.ActiveAllyTeams());

	for (CUnit* unit: units) {
		spring::VectorInsertUnique(teamUnits[unit->allyteam], unit, false);
	}
#endif
}

void CQuadField::Init(int2 mapDims, int quadSize)
{
	quadSizeX = quadSize;
	quadSizeZ = quadSize;
	numQuadsX = (mapDims.x * SQUARE_SIZE) / quadSize;
	numQuadsZ = (mapDims.y * SQUARE_SIZE) / quadSize;

	assert(numQuadsX >= 1);
	assert(numQuadsZ >= 1);
	assert((mapDims.x * SQUARE_SIZE) % quadSize == 0);
	assert((mapDims.y * SQUARE_SIZE) % quadSize == 0);

	invQuadSize = {1.0f / quadSizeX, 1.0f / quadSizeZ};

	baseQuads.resize(numQuadsX * numQuadsZ);
	tempQuads.ReserveAll(numQuadsX * numQuadsZ);
	tempQuads.ReleaseAll();

#ifndef UNIT_TEST
	for (Quad& quad: baseQuads) {
		quad.Resize(teamHandler.ActiveAllyTeams());
	}
#endif
}

void CQuadField::Kill()
{
	// reuse quads when reloading
	// baseQuads.clear();
	for (Quad& quad: baseQuads) {
		quad.Clear();
	}

	tempUnits.ReleaseAll();
	tempFeatures.ReleaseAll();
	tempProjectiles.ReleaseAll();
	tempSolids.ReleaseAll();
	tempQuads.ReleaseAll();
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
void CQuadField::GetQuads(QuadFieldQuery& qfq, float3 pos, float radius)
{
	pos.AssertNaNs();
	pos.ClampInBounds();
	qfq.quads = tempQuads.ReserveVector();

	const int2 min = WorldPosToQuadField(pos - radius);
	const int2 max = WorldPosToQuadField(pos + radius);

	if (max.y < min.y || max.x < min.x)
		return;

	// qsx and qsz are always equal
	const float maxSqLength = (radius + quadSizeX * 0.72f) * (radius + quadSizeZ * 0.72f);

	for (int z = min.y; z <= max.y; ++z) {
		for (int x = min.x; x <= max.x; ++x) {
			assert(x < numQuadsX);
			assert(z < numQuadsZ);
			const float3 quadPos = float3(x * quadSizeX + quadSizeX * 0.5f, 0, z * quadSizeZ + quadSizeZ * 0.5f);
			if (pos.SqDistance2D(quadPos) < maxSqLength) {
				qfq.quads->push_back(z * numQuadsX + x);
			}
		}
	}

	return;
}


void CQuadField::GetQuadsRectangle(QuadFieldQuery& qfq, const float3& mins, const float3& maxs)
{
	mins.AssertNaNs();
	maxs.AssertNaNs();
	qfq.quads = tempQuads.ReserveVector();

	const int2 min = WorldPosToQuadField(mins);
	const int2 max = WorldPosToQuadField(maxs);

	if (max.y < min.y || max.x < min.x)
		return;

	for (int z = min.y; z <= max.y; ++z) {
		for (int x = min.x; x <= max.x; ++x) {
			assert(x < numQuadsX);
			assert(z < numQuadsZ);
			qfq.quads->push_back(z * numQuadsX + x);
		}
	}

	return;
}
#endif // UNIT_TEST


/// note: this function got an UnitTest, check the tests/ folder!
void CQuadField::GetQuadsOnRay(QuadFieldQuery& qfq, const float3& start, const float3& dir, float length)
{
	dir.AssertNaNs();
	start.AssertNaNs();

	auto& queryQuads = *(qfq.quads = tempQuads.ReserveVector());

	const float3 to = start + (dir * length);

	const bool noXdir = (math::floor(start.x * invQuadSize.x) == math::floor(to.x * invQuadSize.x));
	const bool noZdir = (math::floor(start.z * invQuadSize.y) == math::floor(to.z * invQuadSize.y));


	// special case
	if (noXdir && noZdir) {
		queryQuads.push_back(WorldPosToQuadFieldIdx(start));
		assert(static_cast<unsigned>(queryQuads.back()) < baseQuads.size());
		return;
	}

	// prevent div0
	if (noZdir) {
		int startX = Clamp<int>(start.x * invQuadSize.x, 0, numQuadsX - 1);
		int finalX = Clamp<int>(   to.x * invQuadSize.x, 0, numQuadsX - 1);

		if (finalX < startX)
			std::swap(startX, finalX);

		assert(finalX < numQuadsX);

		const int row = Clamp<int>(start.z * invQuadSize.y, 0, numQuadsZ - 1) * numQuadsX;

		for (unsigned x = startX; x <= finalX; x++) {
			queryQuads.push_back(row + x);
			assert(static_cast<unsigned>(queryQuads.back()) < baseQuads.size());
		}

		return;
	}


	// iterate z-range; compute which columns (x) are touched for each row (z)
	float startZuc = start.z * invQuadSize.y;
	float finalZuc =    to.z * invQuadSize.y;

	if (finalZuc < startZuc)
		std::swap(startZuc, finalZuc);

	const int startZ = Clamp<int>(startZuc, 0, numQuadsZ - 1);
	const int finalZ = Clamp<int>(finalZuc, 0, numQuadsZ - 1);

	assert(finalZ < quadSizeZ);

	const float invDirZ = 1.0f / dir.z;

	for (int z = startZ; z <= finalZ; z++) {
		float t0 = ((z    ) * quadSizeZ - start.z) * invDirZ;
		float t1 = ((z + 1) * quadSizeZ - start.z) * invDirZ;

		if ((startZuc < 0 && z == 0) || (startZuc >= numQuadsZ && z == finalZ))
			t0 = ((startZuc    ) * quadSizeZ - start.z) * invDirZ;

		if ((finalZuc < 0 && z == 0) || (finalZuc >= numQuadsZ && z == finalZ))
			t1 = ((finalZuc + 1) * quadSizeZ - start.z) * invDirZ;

		t0 = Clamp(t0, 0.0f, length);
		t1 = Clamp(t1, 0.0f, length);

		unsigned startX = Clamp<int>((dir.x * t0 + start.x) * invQuadSize.x, 0, numQuadsX - 1);
		unsigned finalX = Clamp<int>((dir.x * t1 + start.x) * invQuadSize.x, 0, numQuadsX - 1);

		if (finalX < startX)
			std::swap(startX, finalX);

		assert(finalX < numQuadsX);

		const int row = Clamp(z, 0, numQuadsZ - 1) * numQuadsX;

		for (unsigned x = startX; x <= finalX; x++) {
			queryQuads.push_back(row + x);
			assert(static_cast<unsigned>(queryQuads.back()) < baseQuads.size());
		}
	}
}



#ifndef UNIT_TEST
bool CQuadField::InsertUnitIf(CUnit* unit, const float3& wpos)
{
	assert(unit != nullptr);

	const int wposQuadIdx = WorldPosToQuadFieldIdx(wpos);
	const int uposQuadIdx = WorldPosToQuadFieldIdx(unit->pos);

	// do nothing if unit already exists in cell containing <wpos>
	if (wposQuadIdx == uposQuadIdx)
		return false;

	// unit might also be overlapping the cell, so test for uniqueness
	if (!spring::VectorInsertUnique(unit->quads, wposQuadIdx, true))
		return false;

	spring::VectorInsertUnique(baseQuads[wposQuadIdx].units, unit, false);
	spring::VectorInsertUnique(baseQuads[wposQuadIdx].teamUnits[unit->allyteam], unit, false);
	return true;
}

bool CQuadField::RemoveUnitIf(CUnit* unit, const float3& wpos)
{
	if (unit == nullptr)
		return false;

	const int wposQuadIdx = WorldPosToQuadFieldIdx(wpos);
	const int uposQuadIdx = WorldPosToQuadFieldIdx(unit->pos);

	// do nothing if unit now exists in cell containing <wpos>
	// (meaning it must have somehow moved since InsertUnitIf)
	if (wposQuadIdx == uposQuadIdx)
		return false;

	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, unit->pos, unit->radius);

	// do nothing if the cells touched by unit now contain <wpos>
	if (std::find(qfQuery.quads->begin(), qfQuery.quads->end(), wposQuadIdx) != qfQuery.quads->end()) {
		assert(std::find(unit->quads.begin(), unit->quads.end(), wposQuadIdx) != unit->quads.end());
		return false;
	}

	if (!spring::VectorErase(unit->quads, wposQuadIdx))
		return false;

	spring::VectorErase(baseQuads[wposQuadIdx].units, unit);
	spring::VectorErase(baseQuads[wposQuadIdx].teamUnits[unit->allyteam], unit);
	return true;
}
#endif



#ifndef UNIT_TEST
void CQuadField::MovedUnit(CUnit* unit)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, unit->pos, unit->radius);

	// compare if the quads have changed, if not stop here
	if (qfQuery.quads->size() == unit->quads.size()) {
		if (std::equal(qfQuery.quads->begin(), qfQuery.quads->end(), unit->quads.begin()))
			return;
	}

	for (const int qi: unit->quads) {
		spring::VectorErase(baseQuads[qi].units, unit);
		spring::VectorErase(baseQuads[qi].teamUnits[unit->allyteam], unit);
	}

	for (const int qi: *qfQuery.quads) {
		spring::VectorInsertUnique(baseQuads[qi].units, unit, false);
		spring::VectorInsertUnique(baseQuads[qi].teamUnits[unit->allyteam], unit, false);
	}

	unit->quads = std::move(*qfQuery.quads);
}

void CQuadField::RemoveUnit(CUnit* unit)
{
	for (const int qi: unit->quads) {
		spring::VectorErase(baseQuads[qi].units, unit);
		spring::VectorErase(baseQuads[qi].teamUnits[unit->allyteam], unit);
	}

	unit->quads.clear();

	#ifdef DEBUG_QUADFIELD
	for (const Quad& q: baseQuads) {
		for (auto& teamUnits: q.teamUnits) {
			for (CUnit* u: teamUnits) {
				assert(u != unit);
			}
		}
	}
	#endif
}


void CQuadField::MovedRepulser(CPlasmaRepulser* repulser)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, repulser->weaponMuzzlePos, repulser->GetRadius());

	const auto& repulserQuads = repulser->GetQuads();

	// compare if the quads have changed, if not stop here
	if (qfQuery.quads->size() == repulserQuads.size()) {
		if (std::equal(qfQuery.quads->begin(), qfQuery.quads->end(), repulserQuads.begin()))
			return;
	}

	for (const int qi: repulserQuads) {
		spring::VectorErase(baseQuads[qi].repulsers, repulser);
	}

	for (const int qi: *qfQuery.quads) {
		spring::VectorInsertUnique(baseQuads[qi].repulsers, repulser, false);
	}

	repulser->SetQuads(std::move(*qfQuery.quads));
}

void CQuadField::RemoveRepulser(CPlasmaRepulser* repulser)
{
	for (const int qi: repulser->GetQuads()) {
		spring::VectorErase(baseQuads[qi].repulsers, repulser);
	}

	repulser->ClearQuads();

	#ifdef DEBUG_QUADFIELD
	for (const Quad& q: baseQuads) {
		for (CPlasmaRepulser* r: q.repulsers) {
			assert(r != repulser);
		}
	}
	#endif
}


void CQuadField::AddFeature(CFeature* feature)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, feature->pos, feature->radius);

	for (const int qi: *qfQuery.quads) {
		spring::VectorInsertUnique(baseQuads[qi].features, feature, false);
	}
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, feature->pos, feature->radius);

	for (const int qi: *qfQuery.quads) {
		spring::VectorErase(baseQuads[qi].features, feature);
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
		QuadFieldQuery qfQuery;
		GetQuadsOnRay(qfQuery, p->pos, p->dir, p->speed.w);

		for (const int qi: *qfQuery.quads) {
			spring::VectorInsertUnique(baseQuads[qi].projectiles, p, false);
		}

		p->quads = std::move(*qfQuery.quads);
	} else {
		int newQuad = WorldPosToQuadFieldIdx(p->pos);
		spring::VectorInsertUnique(baseQuads[newQuad].projectiles, p, false);
		p->quads.clear();
		p->quads.push_back(newQuad);
	}
}

void CQuadField::RemoveProjectile(CProjectile* p)
{
	assert(p->synced);

	for (const int qi: p->quads) {
		spring::VectorErase(baseQuads[qi].projectiles, p);
	}

	p->quads.clear();
}





void CQuadField::GetUnits(QuadFieldQuery& qfq, const float3& pos, float radius)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	const int tempNum = gs->GetTempNum();
	qfq.units = tempUnits.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;
			qfq.units->push_back(u);
		}
	}

	return;
}

void CQuadField::GetUnitsExact(QuadFieldQuery& qfq, const float3& pos, float radius, bool spherical)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	const int tempNum = gs->GetTempNum();
	qfq.units = tempUnits.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;

			const float totRad       = radius + u->radius;
			const float totRadSq     = totRad * totRad;
			const float posUnitDstSq = spherical?
				pos.SqDistance(u->pos):
				pos.SqDistance2D(u->pos);

			if (posUnitDstSq >= totRadSq)
				continue;

			qfq.units->push_back(u);
		}
	}

	return;
}

void CQuadField::GetUnitsExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs)
{
	QuadFieldQuery qfQuery;
	GetQuadsRectangle(qfQuery, mins, maxs);
	const int tempNum = gs->GetTempNum();
	qfq.units = tempUnits.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CUnit* unit: baseQuads[qi].units) {

			if (unit->tempNum == tempNum)
				continue;

			unit->tempNum = tempNum;

			const float3& pos = unit->pos;
			if (pos.x < mins.x || pos.x > maxs.x)
				continue;
			if (pos.z < mins.z || pos.z > maxs.z)
				continue;

			qfq.units->push_back(unit);
		}
	}

	return;
}


void CQuadField::GetFeaturesExact(QuadFieldQuery& qfq, const float3& pos, float radius, bool spherical)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	const int tempNum = gs->GetTempNum();
	qfq.features = tempFeatures.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CFeature* f: baseQuads[qi].features) {
			if (f->tempNum == tempNum)
				continue;

			f->tempNum = tempNum;

			const float totRad       = radius + f->radius;
			const float totRadSq     = totRad * totRad;
			const float posDstSq = spherical?
				pos.SqDistance(f->pos):
				pos.SqDistance2D(f->pos);

			if (posDstSq >= totRadSq)
				continue;

			qfq.features->push_back(f);
		}
	}

	return;
}

void CQuadField::GetFeaturesExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs)
{
	QuadFieldQuery qfQuery;
	GetQuadsRectangle(qfQuery, mins, maxs);
	const int tempNum = gs->GetTempNum();
	qfq.features = tempFeatures.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CFeature* feature: baseQuads[qi].features) {
			if (feature->tempNum == tempNum)
				continue;

			feature->tempNum = tempNum;

			const float3& pos = feature->pos;
			if (pos.x < mins.x || pos.x > maxs.x)
				continue;
			if (pos.z < mins.z || pos.z > maxs.z)
				continue;

			qfq.features->push_back(feature);
		}
	}

	return;
}



void CQuadField::GetProjectilesExact(QuadFieldQuery& qfq, const float3& pos, float radius)
{
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	const int tempNum = gs->GetTempNum();
	qfq.projectiles = tempProjectiles.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CProjectile* p: baseQuads[qi].projectiles) {
			if (p->tempNum == tempNum)
				continue;

			p->tempNum = tempNum;

			if (pos.SqDistance(p->pos) >= Square(radius + p->radius))
				continue;

			qfq.projectiles->push_back(p);
		}
	}

	return;
}

void CQuadField::GetProjectilesExact(QuadFieldQuery& qfq, const float3& mins, const float3& maxs)
{
	QuadFieldQuery qfQuery;
	GetQuadsRectangle(qfQuery, mins, maxs);
	const int tempNum = gs->GetTempNum();
	qfq.projectiles = tempProjectiles.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CProjectile* p: baseQuads[qi].projectiles) {
			if (p->tempNum == tempNum)
				continue;

			p->tempNum = tempNum;

			const float3& pos = p->pos;
			if (pos.x < mins.x || pos.x > maxs.x)
				continue;
			if (pos.z < mins.z || pos.z > maxs.z)
				continue;

			qfq.projectiles->push_back(p);
		}
	}

	return;
}



void CQuadField::GetSolidsExact(
	QuadFieldQuery& qfq,
	const float3& pos,
	const float radius,
	const unsigned int physicalStateBits,
	const unsigned int collisionStateBits
) {
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	const int tempNum = gs->GetTempNum();
	qfq.solids = tempSolids.ReserveVector();

	for (const int qi: *qfQuery.quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;

			if (!u->HasPhysicalStateBit(physicalStateBits))
				continue;
			if (!u->HasCollidableStateBit(collisionStateBits))
				continue;
			if ((pos - u->pos).SqLength() >= Square(radius + u->radius))
				continue;

			qfq.solids->push_back(u);
		}

		for (CFeature* f: baseQuads[qi].features) {
			if (f->tempNum == tempNum)
				continue;

			f->tempNum = tempNum;

			if (!f->HasPhysicalStateBit(physicalStateBits))
				continue;
			if (!f->HasCollidableStateBit(collisionStateBits))
				continue;
			if ((pos - f->pos).SqLength() >= Square(radius + f->radius))
				continue;

			qfq.solids->push_back(f);
		}
	}

	return;
}


bool CQuadField::NoSolidsExact(
	const float3& pos,
	const float radius,
	const unsigned int physicalStateBits,
	const unsigned int collisionStateBits
) {
	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	const int tempNum = gs->GetTempNum();

	for (const int qi: *qfQuery.quads) {
		for (CUnit* u: baseQuads[qi].units) {
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;

			if (!u->HasPhysicalStateBit(physicalStateBits))
				continue;
			if (!u->HasCollidableStateBit(collisionStateBits))
				continue;
			if ((pos - u->pos).SqLength() >= Square(radius + u->radius))
				continue;

			return false;
		}

		for (CFeature* f: baseQuads[qi].features) {
			if (f->tempNum == tempNum)
				continue;

			f->tempNum = tempNum;

			if (!f->HasPhysicalStateBit(physicalStateBits))
				continue;
			if (!f->HasCollidableStateBit(collisionStateBits))
				continue;
			if ((pos - f->pos).SqLength() >= Square(radius + f->radius))
				continue;

			return false;
		}
	}

	return true;
}


// optimization specifically for projectile collisions
void CQuadField::GetUnitsAndFeaturesColVol(
	const float3& pos,
	const float radius,
	std::vector<CUnit*>& units,
	std::vector<CFeature*>& features,
	std::vector<CPlasmaRepulser*>* repulsers
) {
	const int tempNum = gs->GetTempNum();

	QuadFieldQuery qfQuery;
	GetQuads(qfQuery, pos, radius);
	// start counting from the previous object-cache sizes

	for (const int qi: *qfQuery.quads) {
		const Quad& quad = baseQuads[qi];

		for (CUnit* u: quad.units) {
			// prevent double adding
			if (u->tempNum == tempNum)
				continue;

			u->tempNum = tempNum;

			const auto* colvol = &u->collisionVolume;
			const float totRad = radius + colvol->GetBoundingRadius();

			if (pos.SqDistance(colvol->GetWorldSpacePos(u)) >= (totRad * totRad))
				continue;

			units.push_back(u);
		}

		for (CFeature* f: quad.features) {
			// prevent double adding
			if (f->tempNum == tempNum)
				continue;

			f->tempNum = tempNum;

			const auto* colvol = &f->collisionVolume;
			const float totRad = radius + colvol->GetBoundingRadius();

			if (pos.SqDistance(colvol->GetWorldSpacePos(f)) >= (totRad * totRad))
				continue;

			features.push_back(f);
		}
		if (repulsers != nullptr) {
			for (CPlasmaRepulser* r: quad.repulsers) {
				// prevent double adding
				if (r->tempNum == tempNum)
					continue;

				r->tempNum = tempNum;

				const auto* colvol = &r->collisionVolume;
				const float totRad = radius + colvol->GetBoundingRadius();

				if (pos.SqDistance(r->weaponMuzzlePos) >= (totRad * totRad))
					continue;

				repulsers->push_back(r);
			}
		}
	}
}
#endif // UNIT_TEST
