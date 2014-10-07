/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include <algorithm>

#include "QuadField.h"
#include "Sim/Misc/CollisionVolume.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Projectiles/Projectile.h"
#include "System/creg/STL_List.h"

#define CELL_IDX_X(wpx) Clamp(int((wpx) / quadSizeX), 0, numQuadsX - 1)
#define CELL_IDX_Z(wpz) Clamp(int((wpz) / quadSizeZ), 0, numQuadsZ - 1)

CR_BIND(CQuadField, (1, 1))
CR_REG_METADATA(CQuadField, (
	CR_MEMBER(baseQuads),
	CR_MEMBER(tempQuads),
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



void CQuadField::Resize(unsigned int nqx, unsigned int nqz)
{
	CQuadField* oldQuadField = quadField;
	CQuadField* newQuadField = new CQuadField(nqx, nqz);

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



CQuadField::Quad::Quad() : teamUnits(teamHandler->ActiveAllyTeams())
{
}

CQuadField::CQuadField(unsigned int nqx, unsigned int nqz)
{
	numQuadsX = nqx;
	numQuadsZ = nqz;
	quadSizeX = (gs->mapx * SQUARE_SIZE) / numQuadsX;
	quadSizeZ = (gs->mapy * SQUARE_SIZE) / numQuadsZ;

	assert(numQuadsX >= 1);
	assert(numQuadsZ >= 1);
	// square cells only
	assert(quadSizeX == quadSizeZ);

	// Without the temporary, std::max takes address of NUM_TEMP_QUADS
	// if it isn't inlined, forcing NUM_TEMP_QUADS to be defined.
	const int numTempQuads = NUM_TEMP_QUADS;

	baseQuads.resize(numQuadsX * numQuadsZ);
	tempQuads.resize(std::max(numTempQuads, numQuadsX * numQuadsZ));
}

CQuadField::~CQuadField()
{
	baseQuads.clear();
	tempQuads.clear();
}


std::vector<int> CQuadField::GetQuads(float3 pos, float radius) const
{
	pos.ClampInBounds();
	pos.AssertNaNs();

	std::vector<int> ret;

	// qsx and qsz are always equal
	const float maxSqLength = (radius + quadSizeX * 0.72f) * (radius + quadSizeZ * 0.72f);

	const int maxx = std::min((int(pos.x + radius)) / quadSizeX + 1, numQuadsX - 1);
	const int maxz = std::min((int(pos.z + radius)) / quadSizeZ + 1, numQuadsZ - 1);

	const int minx = std::max((int(pos.x - radius)) / quadSizeX, 0);
	const int minz = std::max((int(pos.z - radius)) / quadSizeZ, 0);

	if (maxz < minz || maxx < minx) {
		return ret;
	}

	ret.reserve((maxz - minz) * (maxx - minx));

	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
			if ((pos - float3(x * quadSizeX + quadSizeX * 0.5f, 0, z * quadSizeZ + quadSizeZ * 0.5f)).SqLength2D() < maxSqLength) {
				ret.push_back(z * numQuadsX + x);
			}
		}
	}

	return ret;
}


unsigned int CQuadField::GetQuads(float3 pos, float radius, int*& begQuad, int*& endQuad) const
{
	pos.ClampInBounds();
	pos.AssertNaNs();

	assert(begQuad == &tempQuads[0]);
	assert(endQuad == &tempQuads[0]);

	const int maxx = std::min((int(pos.x + radius)) / quadSizeX + 1, numQuadsX - 1);
	const int maxz = std::min((int(pos.z + radius)) / quadSizeZ + 1, numQuadsZ - 1);

	const int minx = std::max((int(pos.x - radius)) / quadSizeX, 0);
	const int minz = std::max((int(pos.z - radius)) / quadSizeZ, 0);

	if (maxz < minz || maxx < minx) {
		return 0;
	}

	// qsx and qsz are always equal
	const float maxSqLength = (radius + quadSizeX * 0.72f) * (radius + quadSizeZ * 0.72f);

	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
			const float3 quadCenterPos = float3(x * quadSizeX + quadSizeX * 0.5f, 0, z * quadSizeZ + quadSizeZ * 0.5f);

			if ((pos - quadCenterPos).SqLength2D() < maxSqLength) {
				*endQuad = z * numQuadsX + x; ++endQuad;
			}
		}
	}

	return (endQuad - begQuad);
}



std::vector<CUnit*> CQuadField::GetUnits(const float3& pos, float radius)
{
	const int tempNum = gs->tempNum++;

	int* begQuad = &tempQuads[0];
	int* endQuad = &tempQuads[0];

	GetQuads(pos, radius, begQuad, endQuad);

	std::vector<CUnit*> units;
	std::list<CUnit*>::iterator ui;

	for (int* a = begQuad; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum == tempNum)
				continue;

			(*ui)->tempNum = tempNum;
			units.push_back(*ui);
		}
	}

	return units;
}

std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& pos, float radius, bool spherical)
{
	const int tempNum = gs->tempNum++;

	int* begQuad = &tempQuads[0];
	int* endQuad = &tempQuads[0];

	GetQuads(pos, radius, begQuad, endQuad);

	std::vector<CUnit*> units;
	std::list<CUnit*>::iterator ui;

	for (int* a = begQuad; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum == tempNum)
				continue;

			const float totRad       = radius + (*ui)->radius;
			const float totRadSq     = totRad * totRad;
			const float posUnitDstSq = spherical?
				pos.SqDistance((*ui)->midPos):
				pos.SqDistance2D((*ui)->midPos);

			if (posUnitDstSq >= totRadSq)
				continue;

			(*ui)->tempNum = tempNum;
			units.push_back(*ui);
		}
	}

	return units;
}

std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& mins, const float3& maxs)
{
	const std::vector<int>& quads = GetQuadsRectangle(mins, maxs);
	const int tempNum = gs->tempNum++;

	std::vector<CUnit*> units;
	std::vector<int>::const_iterator qi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CUnit*>& quadUnits = baseQuads[*qi].units;
		std::list<CUnit*>::iterator ui;

		for (ui = quadUnits.begin(); ui != quadUnits.end(); ++ui) {
			CUnit* unit = *ui;
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



unsigned int CQuadField::GetQuadsOnRay(float3 start, float3 dir, float length, int*& begQuad, int*& endQuad)
{
	assert(!math::isnan(start.x));
	assert(!math::isnan(start.y));
	assert(!math::isnan(start.z));
	assert(!math::isnan(dir.x));
	assert(!math::isnan(dir.y));
	assert(!math::isnan(dir.z));
	assert(begQuad == NULL);
	assert(endQuad == NULL);

	begQuad = &tempQuads[0];
	endQuad = &tempQuads[0];

	if (start.x < 1) {
		if (dir.x == 0) {
			dir.x = 0.00001f;
		}
		start = start + dir * ((1 - start.x) / dir.x);
	}
	if (start.x > gs->mapx * SQUARE_SIZE - 1) {
		if (dir.x == 0) {
			dir.x = 0.00001f;
		}
		start = start + dir * ((gs->mapx * SQUARE_SIZE - 1 - start.x) / dir.x);
	}
	if (start.z < 1) {
		if (dir.z == 0) {
			dir.z = 0.00001f;
		}
		start = start + dir * ((1 - start.z) / dir.z);
	}
	if (start.z > gs->mapy * SQUARE_SIZE - 1) {
		if (dir.z == 0) {
			dir.z = 0.00001f;
		}
		start = start + dir * ((gs->mapy * SQUARE_SIZE - 1 - start.z) / dir.z);
	}

	if (start.x < 1) {
		start.x = 1;
	}
	if (start.x > gs->mapx * SQUARE_SIZE - 1) {
		start.x = gs->mapx * SQUARE_SIZE - 1;
	}
	if (start.z < 1) {
		start.z = 1;
	}
	if (start.z > gs->mapy * SQUARE_SIZE - 1) {
		start.z = gs->mapy * SQUARE_SIZE - 1;
	}

	float3 to = start + (dir * length);

	if (to.x < 1) {
		to = to - dir * ((to.x - 1)                          / dir.x);
	}
	if (to.x > gs->mapx * SQUARE_SIZE - 1) {
		to = to - dir * ((to.x - gs->mapx * SQUARE_SIZE + 1) / dir.x);
	}
	if (to.z < 1){
		to= to - dir * ((to.z - 1)                          / dir.z);
	}
	if (to.z > gs->mapy * SQUARE_SIZE - 1) {
		to= to - dir * ((to.z - gs->mapy * SQUARE_SIZE + 1) / dir.z);
	}
	// these 4 shouldnt be needed, but sometimes we seem to get strange enough
	// values that rounding errors throw us outside the map
	if (to.x < 1) {
		to.x = 1;
	}
	if (to.x > gs->mapx * SQUARE_SIZE - 1) {
		to.x = gs->mapx * SQUARE_SIZE - 1;
	}
	if (to.z < 1) {
		to.z = 1;
	}
	if (to.z > gs->mapy * SQUARE_SIZE - 1) {
		to.z = gs->mapy * SQUARE_SIZE - 1;
	}

	const float dx = to.x - start.x;
	const float dz = to.z - start.z;
	float xp = start.x;
	float zp = start.z;

	const float invQuadSizeX = 1.0f / quadSizeX;
	const float invQuadSizeZ = 1.0f / quadSizeZ;

	if ((math::floor(start.x * invQuadSizeX) == math::floor(to.x * invQuadSizeX)) &&
		(math::floor(start.z * invQuadSizeZ) == math::floor(to.z * invQuadSizeZ)))
	{
		*endQuad = ((int(start.x * invQuadSizeX)) + (int(start.z * invQuadSizeZ)) * numQuadsX);
		++endQuad;
	} else if (math::floor(start.x * invQuadSizeX) == math::floor(to.x * invQuadSizeX)) {
		const int first = (int)(start.x * invQuadSizeX) + ((int)(start.z * invQuadSizeZ) * numQuadsX);
		const int last  = (int)(to.x    * invQuadSizeX) + ((int)(to.z    * invQuadSizeZ) * numQuadsX);

		if (dz > 0) {
			for (int a = first; a <= last; a += numQuadsX) {
				*endQuad = a; ++endQuad;
			}
		} else {
			for(int a = first; a >= last; a -= numQuadsX) {
				*endQuad = a; ++endQuad;
			}
		}
	} else if (math::floor(start.z * invQuadSizeZ) == math::floor(to.z * invQuadSizeZ)) {
		const int first = (int)(start.x * invQuadSizeX) + ((int)(start.z * invQuadSizeZ) * numQuadsX);
		const int last  = (int)(to.x    * invQuadSizeX) + ((int)(to.z    * invQuadSizeZ) * numQuadsX);

		if (dx > 0) {
			for (int a = first; a <= last; a++) {
				*endQuad = a; ++endQuad;
			}
		} else {
			for (int a = first; a >= last; a--) {
				*endQuad = a; ++endQuad;
			}
		}
	} else {
		float xn, zn;
		bool keepgoing = true;

		for (unsigned int i = 0; i < tempQuads.size() && keepgoing; i++) {
			*endQuad = ((int)(zp * invQuadSizeZ) * numQuadsX) + (int)(xp * invQuadSizeX);
			++endQuad;

			if (dx > 0) {
				xn = (math::floor(xp * invQuadSizeX) * quadSizeX + quadSizeX - xp) / dx;
			} else {
				xn = (math::floor(xp * invQuadSizeX) * quadSizeX - xp) / dx;
			}
			if (dz > 0) {
				zn = (math::floor(zp * invQuadSizeZ) * quadSizeZ + quadSizeZ - zp) / dz;
			} else {
				zn = (math::floor(zp * invQuadSizeZ) * quadSizeZ - zp) / dz;
			}

			if (xn < zn) {
				xp += (xn + 0.0001f) * dx;
				zp += (xn + 0.0001f) * dz;
			} else {
				xp += (zn + 0.0001f) * dx;
				zp += (zn + 0.0001f) * dz;
			}

			keepgoing =
				(math::fabs(xp - start.x) < math::fabs(to.x - start.x)) &&
				(math::fabs(zp - start.z) < math::fabs(to.z - start.z));
		}
	}

	return (endQuad - begQuad);
}



void CQuadField::MovedUnit(CUnit* unit)
{
	const std::vector<int>& newQuads = GetQuads(unit->pos, unit->radius);

	// compare if the quads have changed, if not stop here
	if (newQuads.size() == unit->quads.size()) {
		if (std::equal(newQuads.begin(), newQuads.end(), unit->quads.begin())) {
			return;
		}
	}

	std::vector<int>::const_iterator qi;
	for (qi = unit->quads.begin(); qi != unit->quads.end(); ++qi) {
		std::list<CUnit*>& quadUnits     = baseQuads[*qi].units;
		std::list<CUnit*>& quadAllyUnits = baseQuads[*qi].teamUnits[unit->allyteam];
		std::list<CUnit*>::iterator ui;

		ui = std::find(quadUnits.begin(), quadUnits.end(), unit);
		if (ui != quadUnits.end())
			quadUnits.erase(ui);

		ui = std::find(quadAllyUnits.begin(), quadAllyUnits.end(), unit);
		if (ui != quadAllyUnits.end())
			quadAllyUnits.erase(ui);
	}

	for (qi = newQuads.begin(); qi != newQuads.end(); ++qi) {
		baseQuads[*qi].units.push_front(unit);
		baseQuads[*qi].teamUnits[unit->allyteam].push_front(unit);
	}
	unit->quads = newQuads;
}

void CQuadField::RemoveUnit(CUnit* unit)
{
	std::vector<int>::const_iterator qi;
	for (qi = unit->quads.begin(); qi != unit->quads.end(); ++qi) {
		std::list<CUnit*>& quadUnits     = baseQuads[*qi].units;
		std::list<CUnit*>& quadAllyUnits = baseQuads[*qi].teamUnits[unit->allyteam];
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
	const std::vector<int>& newQuads = GetQuads(feature->pos, feature->radius);

	std::vector<int>::const_iterator qi;
	for (qi = newQuads.begin(); qi != newQuads.end(); ++qi) {
		baseQuads[*qi].features.push_front(feature);
	}
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	const std::vector<int>& quads = GetQuads(feature->pos, feature->radius);

	std::vector<int>::const_iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		baseQuads[*qi].features.remove(feature);
	}

	#ifdef DEBUG_QUADFIELD
	for (int x = 0; x < numQuadsX; x++) {
		for (int z = 0; z < numQuadsZ; z++) {
			const Quad& q = baseQuads[z * numQuadsX + x];
			const std::list<CFeature*>& f = q.features;

			std::list<CFeature*>::const_iterator fIt;

			for (fIt = f.begin(); fIt != f.end(); ++fIt) {
				assert((*fIt) != feature);
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

	typedef CQuadField::Quad Cell;
	typedef std::list<CProjectile*> List;

	if (p->hitscan) {
		// all coordinates always map to a valid quad
		qfcd.SetCoor(0, int2(CELL_IDX_X(p->pos.x                    ), CELL_IDX_Z(p->pos.z                    )));
		qfcd.SetCoor(1, int2(CELL_IDX_X(p->pos.x + p->speed.x * 0.5f), CELL_IDX_Z(p->pos.z + p->speed.z * 0.5f)));
		qfcd.SetCoor(2, int2(CELL_IDX_X(p->pos.x + p->speed.x       ), CELL_IDX_Z(p->pos.z + p->speed.z       )));

		Cell& cell = baseQuads[numQuadsX * qfcd.GetCoor(0).y + qfcd.GetCoor(0).x];
		List& list = cell.projectiles;

		// projectiles are point-objects so they exist
		// only in a single cell EXCEPT hit-scan types
		qfcd.SetIter(0, list.insert(list.end(), p));

		for (unsigned int n = 1; n < 3; n++) {
			Cell& ncell = baseQuads[numQuadsX * qfcd.GetCoor(n).y + qfcd.GetCoor(n).x];
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
		qfcd.SetCoor(0, int2(CELL_IDX_X(p->pos.x), CELL_IDX_Z(p->pos.z)));

		Cell& cell = baseQuads[numQuadsX * qfcd.GetCoor(0).y + qfcd.GetCoor(0).x];
		List& list = cell.projectiles;

		qfcd.SetIter(0, list.insert(list.end(), p));
	}

	p->SetQuadFieldCellData(qfcd);
}

void CQuadField::RemoveProjectile(CProjectile* p)
{
	assert(p->synced);

	CProjectile::QuadFieldCellData& qfcd = p->GetQuadFieldCellData();

	typedef CQuadField::Quad Cell;
	typedef std::list<CProjectile*> List;

	if (p->hitscan) {
		for (unsigned int n = 0; n < 3; n++) {
			Cell& cell = baseQuads[numQuadsX * qfcd.GetCoor(n).y + qfcd.GetCoor(n).x];
			List& list = cell.projectiles;

			if (qfcd.GetIter(n) != list.end()) {
				// this is O(1) instead of O(n) and crucially
				// important for projectiles, but less clean
				list.erase(qfcd.GetIter(n));
			}

			qfcd.SetIter(n, list.end());
		}
	} else {
		Cell& cell = baseQuads[numQuadsX * qfcd.GetCoor(0).y + qfcd.GetCoor(0).x];
		List& list = cell.projectiles;

		assert(qfcd.GetIter(0) != list.end());

		list.erase(qfcd.GetIter(0));
		qfcd.SetIter(0, list.end());
	}
}



std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& pos, float radius)
{
	const std::vector<int>& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;

	std::vector<CFeature*> features;
	std::vector<int>::const_iterator qi;
	std::list<CFeature*>::iterator fi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		for (fi = baseQuads[*qi].features.begin(); fi != baseQuads[*qi].features.end(); ++fi) {
			if ((*fi)->tempNum == tempNum) { continue; }
			if (pos.SqDistance((*fi)->midPos) >= Square(radius + (*fi)->radius)) { continue; }

			(*fi)->tempNum = tempNum;
			features.push_back(*fi);
		}
	}

	return features;
}

std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& pos, float radius, bool spherical)
{
	const std::vector<int>& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;

	std::vector<CFeature*> features;
	std::vector<int>::const_iterator qi;
	std::list<CFeature*>::iterator fi;
	const float totRadSq = radius * radius;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		for (fi = baseQuads[*qi].features.begin(); fi != baseQuads[*qi].features.end(); ++fi) {

			if ((*fi)->tempNum == tempNum) { continue; }
			if ((spherical ?
				(pos - (*fi)->midPos).SqLength() :
				(pos - (*fi)->midPos).SqLength2D()) >= totRadSq) { continue; }

			(*fi)->tempNum = tempNum;
			features.push_back(*fi);
		}
	}

	return features;
}

std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& mins, const float3& maxs)
{
	const std::vector<int>& quads = GetQuadsRectangle(mins, maxs);
	const int tempNum = gs->tempNum++;

	std::vector<CFeature*> features;
	std::vector<int>::const_iterator qi;
	std::list<CFeature*>::iterator fi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CFeature*>& quadFeatures = baseQuads[*qi].features;

		for (fi = quadFeatures.begin(); fi != quadFeatures.end(); ++fi) {
			CFeature* feature = *fi;
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
	const std::vector<int>& quads = GetQuads(pos, radius);

	std::vector<CProjectile*> projectiles;
	std::vector<int>::const_iterator qi;
	std::list<CProjectile*>::iterator pi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CProjectile*>& quadProjectiles = baseQuads[*qi].projectiles;

		for (pi = quadProjectiles.begin(); pi != quadProjectiles.end(); ++pi) {
			if ((pos - (*pi)->pos).SqLength() >= Square(radius + (*pi)->radius)) {
				continue;
			}

			projectiles.push_back(*pi);
		}
	}

	return projectiles;
}

std::vector<CProjectile*> CQuadField::GetProjectilesExact(const float3& mins, const float3& maxs)
{
	const std::vector<int>& quads = GetQuadsRectangle(mins, maxs);

	std::vector<CProjectile*> projectiles;
	std::vector<int>::const_iterator qi;
	std::list<CProjectile*>::iterator pi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CProjectile*>& quadProjectiles = baseQuads[*qi].projectiles;

		for (pi = quadProjectiles.begin(); pi != quadProjectiles.end(); ++pi) {
			CProjectile* projectile = *pi;
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
	const std::vector<int>& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;

	std::vector<CSolidObject*> solids;
	std::vector<int>::const_iterator qi;

	std::list<CUnit*>::iterator ui;
	std::list<CFeature*>::iterator fi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			CUnit* u = *ui;

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

		for (fi = baseQuads[*qi].features.begin(); fi != baseQuads[*qi].features.end(); ++fi) {
			CFeature* f = *fi;

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



std::vector<int> CQuadField::GetQuadsRectangle(const float3& pos1, const float3& pos2) const
{
	assert(!math::isnan(pos1.x));
	assert(!math::isnan(pos1.y));
	assert(!math::isnan(pos1.z));
	assert(!math::isnan(pos2.x));
	assert(!math::isnan(pos2.y));
	assert(!math::isnan(pos2.z));

	std::vector<int> ret;

	const int maxx = std::max(0, std::min((int(pos2.x)) / quadSizeX + 1, numQuadsX - 1));
	const int maxz = std::max(0, std::min((int(pos2.z)) / quadSizeZ + 1, numQuadsZ - 1));

	const int minx = std::max(0, std::min((int(pos1.x)) / quadSizeX, numQuadsX - 1));
	const int minz = std::max(0, std::min((int(pos1.z)) / quadSizeZ, numQuadsZ - 1));

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

	int* begQuad = &tempQuads[0];
	int* endQuad = &tempQuads[0];

	// bail early if caches are already full
	if (numUnits >= units.size())
		return;
	if (numFeatures >= features.size())
		return;

	assert(numUnits == 0 || units[numUnits] == NULL);
	assert(numFeatures == 0 || features[numFeatures] == NULL);

	GetQuads(pos, radius, begQuad, endQuad);

	std::list<CUnit*>::const_iterator ui;
	std::list<CFeature*>::const_iterator fi;

	for (int* a = begQuad; a != endQuad; ++a) {
		const Quad& quad = baseQuads[*a];

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			CUnit* u = *ui;

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

		for (fi = quad.features.begin(); fi != quad.features.end(); ++fi) {
			CFeature* f = *fi;

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

	assert(numUnits < units.size());
	assert(numFeatures < features.size());

	// set end-of-list sentinels
	if (numUnits < units.size())
		units[numUnits] = NULL;
	if (numFeatures < features.size())
		features[numFeatures] = NULL;

	if (numUnitsPtr != NULL) { *numUnitsPtr = numUnits; }
	if (numFeaturesPtr != NULL) { *numFeaturesPtr = numFeatures; }
}

