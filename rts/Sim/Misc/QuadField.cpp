/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/StdAfx.h"
#include "System/mmgr.h"

#include "lib/gml/gml.h"
#include "QuadField.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "Sim/Projectiles/Projectile.h"
#include "System/creg/STL_List.h"

#define REMOVE_PROJECTILE_FAST

CR_BIND(CQuadField, );
CR_REG_METADATA(CQuadField, (
	// CR_MEMBER(baseQuads),
	CR_MEMBER(numQuadsX),
	CR_MEMBER(numQuadsZ),
	// CR_MEMBER(tempQuads),
	CR_RESERVED(8),
	CR_SERIALIZER(Serialize)
));


CQuadField::Quad::Quad() : teamUnits(teamHandler->ActiveAllyTeams())
{
}

void CQuadField::Serialize(creg::ISerializer& s)
{
	// no need to alloc quad array, this has already been done in constructor
	for (int z = 0; z < numQuadsZ; ++z) {
		for (int x = 0; x < numQuadsX; ++x) {
			s.SerializeObjectInstance(&baseQuads[(z * numQuadsX) + x], Quad::StaticClass());
		}
	}
}



CR_BIND(CQuadField::Quad, );

CR_REG_METADATA_SUB(CQuadField, Quad, (
	CR_MEMBER(units),
	CR_MEMBER(teamUnits),
	CR_MEMBER(features),
	CR_MEMBER(projectiles)
));


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CQuadField* qf;

CQuadField::CQuadField()
{
	numQuadsX = gs->mapx * SQUARE_SIZE / QUAD_SIZE;
	numQuadsZ = gs->mapy * SQUARE_SIZE / QUAD_SIZE;

	baseQuads.resize(numQuadsX * numQuadsZ);

	tempQuads = new int[std::max(1000, numQuadsX * numQuadsZ)];
}

CQuadField::~CQuadField()
{
	delete[] tempQuads;
}


std::vector<int> CQuadField::GetQuads(float3 pos, float radius) const
{
	pos.CheckInBounds();

	std::vector<int> ret;

	const int maxx = std::min(((int)(pos.x + radius)) / QUAD_SIZE + 1, numQuadsX - 1);
	const int maxz = std::min(((int)(pos.z + radius)) / QUAD_SIZE + 1, numQuadsZ - 1);

	const int minx = std::max(((int)(pos.x - radius)) / QUAD_SIZE, 0);
	const int minz = std::max(((int)(pos.z - radius)) / QUAD_SIZE, 0);

	if (maxz < minz || maxx < minx) {
		return ret;
	}

	const float maxSqLength = (radius + QUAD_SIZE * 0.72f) * (radius + QUAD_SIZE * 0.72f);
	ret.reserve((maxz - minz) * (maxx - minx));
	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
			if ((pos - float3(x * QUAD_SIZE + QUAD_SIZE * 0.5f, 0, z * QUAD_SIZE + QUAD_SIZE * 0.5f)).SqLength2D() < maxSqLength) {
				ret.push_back(z * numQuadsX + x);
			}
		}
	}

	return ret;
}


void CQuadField::GetQuads(float3 pos, float radius, int*& dst) const
{
	pos.CheckInBounds();

	const int maxx = std::min(((int)(pos.x + radius)) / QUAD_SIZE + 1, numQuadsX - 1);
	const int maxz = std::min(((int)(pos.z + radius)) / QUAD_SIZE + 1, numQuadsZ - 1);

	const int minx = std::max(((int)(pos.x - radius)) / QUAD_SIZE, 0);
	const int minz = std::max(((int)(pos.z - radius)) / QUAD_SIZE, 0);

	if (maxz < minz || maxx < minx) {
		return;
	}

	const float maxSqLength = (radius + QUAD_SIZE * 0.72f) * (radius + QUAD_SIZE * 0.72f);
	for (int z = minz; z <= maxz; ++z) {
		for (int x = minx; x <= maxx; ++x) {
			const float3 quadCenterPos = float3(x * QUAD_SIZE + QUAD_SIZE * 0.5f, 0, z * QUAD_SIZE + QUAD_SIZE * 0.5f);

			if ((pos - quadCenterPos).SqLength2D() < maxSqLength) {
				*dst = z * numQuadsX + x;
				++dst;
			}
		}
	}
}



std::vector<CUnit*> CQuadField::GetUnits(const float3& pos, float radius)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnits

	const int tempNum = gs->tempNum++;

	int* endQuad = tempQuads;
	GetQuads(pos, radius, endQuad);

	std::vector<CUnit*> units;
	std::list<CUnit*>::iterator ui;

	for (int* a = tempQuads; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum == tempNum) { continue; }

			(*ui)->tempNum = tempNum;
			units.push_back(*ui);
		}
	}

	return units;
}

std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& pos, float radius, bool spherical)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnitsExact

	const int tempNum = gs->tempNum++;

	int* endQuad = tempQuads;
	GetQuads(pos, radius, endQuad);

	std::vector<CUnit*> units;
	std::list<CUnit*>::iterator ui;

	for (int* a = tempQuads; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum == tempNum) { continue; }

			const float totRad       = radius + (*ui)->radius;
			const float totRadSq     = totRad * totRad;
			const float posUnitDstSq = spherical?
				(pos - (*ui)->midPos).SqLength():
				(pos - (*ui)->midPos).SqLength2D();

			if (posUnitDstSq >= totRadSq) { continue; }

			(*ui)->tempNum = tempNum;
			units.push_back(*ui);
		}
	}

	return units;
}

std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& mins, const float3& maxs)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnitsExact

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



std::vector<int> CQuadField::GetQuadsOnRay(const float3& start, float3 dir, float length)
{
	int* end = tempQuads;
	GetQuadsOnRay(start, dir, length, end);

	return std::vector<int>(tempQuads, end);
}

void CQuadField::GetQuadsOnRay(float3 start, float3 dir, float length, int*& dst)
{
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
	const float invQuadSize = 1.0f / QUAD_SIZE;

	if ((floor(start.x * invQuadSize) == floor(to.x * invQuadSize))
			&& (floor(start.z * invQuadSize) == floor(to.z * invQuadSize)))
	{
		*dst = ((int(start.x * invQuadSize)) + (int(start.z * invQuadSize)) * numQuadsX);
		++dst;
	} else if (floor(start.x * invQuadSize) == floor(to.x * invQuadSize)) {
		const int first = (int)(start.x * invQuadSize) + ((int)(start.z * invQuadSize) * numQuadsX);
		const int last  = (int)(to.x    * invQuadSize) + ((int)(to.z    * invQuadSize) * numQuadsX);
		if (dz > 0) {
			for (int a = first; a <= last; a += numQuadsX) {
				*dst = a;
				++dst;
			}
		} else {
			for(int a = first; a >= last; a -= numQuadsX) {
				*dst = a;
				++dst;
			}
		}
	} else if (floor(start.z * invQuadSize) == floor(to.z * invQuadSize)) {
		const int first = (int)(start.x * invQuadSize) + ((int)(start.z * invQuadSize) * numQuadsX);
		const int last  = (int)(to.x    * invQuadSize) + ((int)(to.z    * invQuadSize) * numQuadsX);
		if (dx > 0) {
			for (int a = first; a <= last; a++) {
				*dst = a;
				++dst;
			}
		} else {
			for (int a = first; a >= last; a--) {
				*dst = a;
				++dst;
			}
		}
	} else {
		float xn, zn;
		bool keepgoing = true;
		for (int i = 0; i < 1000 && keepgoing; i++) {
			*dst = ((int)(zp * invQuadSize) * numQuadsX) + (int)(xp * invQuadSize);
			++dst;

			if (dx > 0) {
				xn = (floor(xp * invQuadSize) * QUAD_SIZE + QUAD_SIZE - xp) / dx;
			} else {
				xn = (floor(xp * invQuadSize) * QUAD_SIZE - xp) / dx;
			}
			if (dz > 0) {
				zn = (floor(zp * invQuadSize) * QUAD_SIZE + QUAD_SIZE - zp) / dz;
			} else {
				zn = (floor(zp * invQuadSize) * QUAD_SIZE - zp) / dz;
			}

			if (xn < zn) {
				xp += (xn + 0.0001f) * dx;
				zp += (xn + 0.0001f) * dz;
			} else {
				xp += (zn + 0.0001f) * dx;
				zp += (zn + 0.0001f) * dz;
			}
			keepgoing = (fabs(xp - start.x) < fabs(to.x - start.x))
					&& (fabs(zp - start.z) < fabs(to.z - start.z));
		}
	}
}



void CQuadField::MovedUnit(CUnit* unit)
{
	const std::vector<int>& newQuads = GetQuads(unit->pos,unit->radius);

	//! compare if the quads have changed, if not stop here
	if (newQuads.size() == unit->quads.size()) {
		std::vector<int>::const_iterator qi1, qi2;
		qi1 = unit->quads.begin();
		for (qi2 = newQuads.begin(); qi2 != newQuads.end(); ++qi2) {
			if (*qi1 != *qi2) {
				break;
			}
			++qi1;
		}
		if (qi2 == newQuads.end()) {
			return;
		}
	}

	GML_RECMUTEX_LOCK(quad); // MovedUnit - possible performance hog

	std::vector<int>::const_iterator qi;
	for (qi = unit->quads.begin(); qi != unit->quads.end(); ++qi) {
		std::list<CUnit*>::iterator ui;
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			if (*ui == unit) {
				baseQuads[*qi].units.erase(ui);
				break;
			}
		}
		for (ui = baseQuads[*qi].teamUnits[unit->allyteam].begin(); ui != baseQuads[*qi].teamUnits[unit->allyteam].end(); ++ui) {
			if (*ui == unit) {
				baseQuads[*qi].teamUnits[unit->allyteam].erase(ui);
				break;
			}
		}
	}
	for (qi = newQuads.begin(); qi != newQuads.end(); ++qi) {
		baseQuads[*qi].units.push_front(unit);
		baseQuads[*qi].teamUnits[unit->allyteam].push_front(unit);
	}
	unit->quads = newQuads;
}

void CQuadField::RemoveUnit(CUnit* unit)
{
	GML_RECMUTEX_LOCK(quad); // RemoveUnit

	std::vector<int>::const_iterator qi;
	for (qi = unit->quads.begin(); qi != unit->quads.end(); ++qi) {
		std::list<CUnit*>::iterator ui;
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			if (*ui == unit) {
				baseQuads[*qi].units.erase(ui);
				break;
			}
		}
		for (ui = baseQuads[*qi].teamUnits[unit->allyteam].begin(); ui != baseQuads[*qi].teamUnits[unit->allyteam].end(); ++ui) {
			if (*ui == unit) {
				baseQuads[*qi].teamUnits[unit->allyteam].erase(ui);
				break;
			}
		}
	}
	unit->quads.clear();
}



void CQuadField::AddFeature(CFeature* feature)
{
	GML_RECMUTEX_LOCK(quad); // AddFeature

	const std::vector<int>& newQuads = GetQuads(feature->pos, feature->radius);

	std::vector<int>::const_iterator qi;
	for (qi = newQuads.begin(); qi != newQuads.end(); ++qi) {
		baseQuads[*qi].features.push_front(feature);
	}
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	GML_RECMUTEX_LOCK(quad); // RemoveFeature

	const std::vector<int>& quads = GetQuads(feature->pos, feature->radius);

	std::vector<int>::const_iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		baseQuads[*qi].features.remove(feature);
	}
}



void CQuadField::MovedProjectile(CProjectile* p)
{
	if (!p->synced)
		return;

	int2 oldCellCoors = p->GetQuadFieldCellCoors();
	int2 newCellCoors;
	newCellCoors.x = std::max(0, std::min(int(p->pos.x / QUAD_SIZE), numQuadsX - 1));
	newCellCoors.y = std::max(0, std::min(int(p->pos.z / QUAD_SIZE), numQuadsZ - 1));

	if (newCellCoors.x != oldCellCoors.x || newCellCoors.y != oldCellCoors.y) {
		RemoveProjectile(p);
		AddProjectile(p);
	}
}

void CQuadField::AddProjectile(CProjectile* p)
{
	assert(p->synced);

	// projectiles are point-objects, so
	// they exist in a single cell only
	int2 cellCoors;
	cellCoors.x = std::max(0, std::min(int(p->pos.x / QUAD_SIZE), numQuadsX - 1));
	cellCoors.y = std::max(0, std::min(int(p->pos.z / QUAD_SIZE), numQuadsZ - 1));

	GML_RECMUTEX_LOCK(quad);

	Quad& q = baseQuads[numQuadsX * cellCoors.y + cellCoors.x];
	std::list<CProjectile*>& projectiles = q.projectiles;

	p->SetQuadFieldCellCoors(cellCoors);
	p->SetQuadFieldCellIter(projectiles.insert(projectiles.end(), p));
}

void CQuadField::RemoveProjectile(CProjectile* p)
{
	assert(p->synced);

	const int2& cellCoors = p->GetQuadFieldCellCoors();
	const int cellIdx = numQuadsX * cellCoors.y + cellCoors.x;

	GML_RECMUTEX_LOCK(quad);

	Quad& q = baseQuads[cellIdx];

	std::list<CProjectile*>& projectiles = q.projectiles;
	std::list<CProjectile*>::iterator pi = p->GetQuadFieldCellIter();

	#ifdef REMOVE_PROJECTILE_FAST
	if (pi != projectiles.end()) {
		// this is O(1) instead of O(n) and crucially
		// important for projectiles, but less clean
		projectiles.erase(pi);
	} else {
		assert(false);
	}
	#else
	for (pi = projectiles.begin(); pi != projectiles.end(); ++pi) {
		if ((*pi) == p) {
			projectiles.erase(pi);
			break;
		}
	}
	#endif

	p->SetQuadFieldCellIter(projectiles.end());
}



std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& pos, float radius)
{
	GML_RECMUTEX_LOCK(qnum); // GetFeaturesExact

	const std::vector<int>& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;

	std::vector<CFeature*> features;
	std::vector<int>::const_iterator qi;
	std::list<CFeature*>::iterator fi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		for (fi = baseQuads[*qi].features.begin(); fi != baseQuads[*qi].features.end(); ++fi) {
			const float totRad = radius + (*fi)->radius;

			if ((*fi)->tempNum == tempNum) { continue; }
			if ((pos - (*fi)->midPos).SqLength() >= (totRad * totRad)) { continue; }

			(*fi)->tempNum = tempNum;
			features.push_back(*fi);
		}
	}

	return features;
}

std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& mins, const float3& maxs)
{
	GML_RECMUTEX_LOCK(qnum); // GetFeaturesExact

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
	GML_RECMUTEX_LOCK(qnum);

	const std::vector<int>& quads = GetQuads(pos, radius);

	std::vector<CProjectile*> projectiles;
	std::vector<int>::const_iterator qi;
	std::list<CProjectile*>::iterator pi;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CProjectile*>& quadProjectiles = baseQuads[*qi].projectiles;

		for (pi = quadProjectiles.begin(); pi != quadProjectiles.end(); ++pi) {
			const float totRad = radius + (*pi)->radius;

			if ((pos - (*pi)->pos).SqLength() >= (totRad * totRad)) {
				continue;
			}

			projectiles.push_back(*pi);
		}
	}

	return projectiles;
}

std::vector<CProjectile*> CQuadField::GetProjectilesExact(const float3& mins, const float3& maxs)
{
	GML_RECMUTEX_LOCK(qnum);

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



std::vector<CSolidObject*> CQuadField::GetSolidsExact(const float3& pos, float radius)
{
	GML_RECMUTEX_LOCK(qnum); // GetSolidsExact

	const std::vector<int>& quads = GetQuads(pos, radius);
	const int tempNum = gs->tempNum++;

	std::vector<CSolidObject*> solids;
	std::vector<int>::const_iterator qi;
	std::list<CUnit*>::iterator ui;

	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			const float totRad = radius + (*ui)->radius;

			if (!(*ui)->blocking) { continue; }
			if ((*ui)->tempNum == tempNum) { continue; }
			if ((pos - (*ui)->midPos).SqLength() >= (totRad * totRad)) { continue; }

			(*ui)->tempNum = tempNum;
			solids.push_back(*ui);
		}

		std::list<CFeature*>::iterator fi;
		for (fi = baseQuads[*qi].features.begin(); fi != baseQuads[*qi].features.end(); ++fi) {
			const float totRad = radius + (*fi)->radius;

			if (!(*fi)->blocking) { continue; }
			if ((*fi)->tempNum == tempNum) { continue; }
			if ((pos - (*fi)->midPos).SqLength() >= (totRad * totRad)) { continue; }

			(*fi)->tempNum = tempNum;
			solids.push_back(*fi);
		}
	}

	return solids;
}



std::vector<int> CQuadField::GetQuadsRectangle(const float3& pos,const float3& pos2) const
{
	std::vector<int> ret;

	const int maxx = std::max(0, std::min(((int)(pos2.x)) / QUAD_SIZE + 1, numQuadsX - 1));
	const int maxz = std::max(0, std::min(((int)(pos2.z)) / QUAD_SIZE + 1, numQuadsZ - 1));

	const int minx = std::max(0, std::min(((int)(pos.x)) / QUAD_SIZE, numQuadsX - 1));
	const int minz = std::max(0, std::min(((int)(pos.z)) / QUAD_SIZE, numQuadsZ - 1));

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
void CQuadField::GetUnitsAndFeaturesExact(const float3& pos, float radius, CUnit**& dstUnit, CFeature**& dstFeature)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnitsAndFeaturesExact

	const int tempNum = gs->tempNum++;

	int* endQuad = tempQuads;
	GetQuads(pos, radius, endQuad);

	std::list<CUnit*>::iterator ui;
	std::list<CFeature*>::iterator fi;

	for (int* a = tempQuads; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum == tempNum) { continue; }

			(*ui)->tempNum = tempNum;
			*dstUnit = *ui;
			++dstUnit;
		}

		for (fi = quad.features.begin(); fi != quad.features.end(); ++fi) {
			const float totRad = radius + (*fi)->radius;

			if ((*fi)->tempNum == tempNum) { continue; }
			if ((pos - (*fi)->midPos).SqLength() >= (totRad * totRad)) { continue; }

			(*fi)->tempNum = tempNum;
			*dstFeature = *fi;
			++dstFeature;
		}
	}
}
