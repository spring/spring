/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "System/creg/STL_Map.h"
#include "GeometricObjects.h"
#include "Map/ReadMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Rendering/Env/Particles/Classes/GeoSquareProjectile.h"

CR_BIND(CGeometricObjects, )
CR_BIND(CGeometricObjects::GeoGroup, )

CR_REG_METADATA(CGeometricObjects, (
	CR_MEMBER(geoGroups),
	CR_MEMBER(timedGroups),
	CR_MEMBER(firstFreeGroup)
))

CR_REG_METADATA_SUB(CGeometricObjects, GeoGroup, (CR_MEMBER(squares)))


CGeometricObjects* geometricObjects;

CGeometricObjects::~CGeometricObjects()
{
	timedGroups.clear();
	while (!geoGroups.empty()) {
		DeleteGroup(geoGroups.begin()->first);
	}
}

int CGeometricObjects::AddSpline(float3 b1, float3 b2, float3 b3, float3 b4, float width, int arrow, int lifeTime, int group)
{
	if (group == 0)
		group = firstFreeGroup++;

	float3 old1, old2;
	old1 = CalcSpline(0.00f, b1, b2, b3, b4);
	old2 = CalcSpline(0.05f, b1, b2, b3, b4);

	for (int a = 0; a < 20; ++a) {
		const float3 np = CalcSpline(a*0.05f + 0.1f, b1, b2, b3, b4);
		const float3 dir1 = (old2 - old1).ANormalize();
		const float3 dir2 = (np - old2).ANormalize();

		float w1, w2;
		if ((arrow == 1) && (a == 19)) {
			w1 = width;
			w2 = 0;
		} else {
			w1 = width * 0.5f;
			w2 = w1;
		}
		CGeoSquareProjectile* gsp = projMemPool.alloc<CGeoSquareProjectile>(old1, old2, dir1, dir2, w1, w2);
		geoGroups[group].squares.push_back(gsp);
		old1 = old2;
		old2 = np;
	}

	if (lifeTime > 0)
		timedGroups[gs->frameNum + lifeTime].push_back(group);

	return group;
}


void CGeometricObjects::DeleteGroup(int group)
{
	GeoGroup* gg = &geoGroups[group];

	for (auto gi = gg->squares.begin(); gi != gg->squares.end(); ++gi) {
		(*gi)->deleteMe = true;
	}

	geoGroups.erase(group);
}


void CGeometricObjects::SetColor(int group, float r, float g, float b, float a)
{
	GeoGroup* gg = &geoGroups[group];

	for (auto gi = gg->squares.begin(); gi != gg->squares.end(); ++gi) {
		(*gi)->SetColor(r, g, b, a);
	}
}


float3 CGeometricObjects::CalcSpline(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4)
{
	float ni = 1 - i;

	float3 res = p1*ni*ni*ni + p2*3*i*ni*ni + p3*3*i*i*ni + p4*i*i*i;
	return res;
}


int CGeometricObjects::AddLine(float3 start, float3 end, float width, int arrow, int lifetime, int group)
{
	if (group == 0)
		group = firstFreeGroup++;

	float3 dir = (end - start).SafeANormalize();
	if (arrow) {
		CGeoSquareProjectile* gsp = projMemPool.alloc<CGeoSquareProjectile>(start, start*0.2f + end*0.8f, dir, dir, width*0.5f, width*0.5f);
		geoGroups[group].squares.push_back(gsp);

		gsp = projMemPool.alloc<CGeoSquareProjectile>(start*0.2f + end*0.8f, end, dir, dir, width, 0);
		geoGroups[group].squares.push_back(gsp);

	} else {
		CGeoSquareProjectile* gsp = projMemPool.alloc<CGeoSquareProjectile>(start, end, dir, dir, width*0.5f, width*0.5f);
		geoGroups[group].squares.push_back(gsp);
	}

	if (lifetime > 0)
		timedGroups[gs->frameNum + lifetime].push_back(group);

	return group;
}


void CGeometricObjects::Update()
{
	const auto iter = timedGroups.find(gs->frameNum);

	if (iter == timedGroups.end())
		return;

	for (const int groupID: iter->second) {
		DeleteGroup(groupID);
	}

	timedGroups.erase(iter->first);
}


void CGeometricObjects::MarkSquare(int mapSquare)
{
	float3 startPos;
	startPos.x = (int) (mapSquare * SQUARE_SIZE) % mapDims.mapx;
	startPos.z = (int) (mapSquare * SQUARE_SIZE) / mapDims.mapx;
	startPos.y = readMap->GetCenterHeightMapSynced()[mapSquare];

	float3 endPos = startPos;
	endPos.x += SQUARE_SIZE;
	endPos.z += SQUARE_SIZE;

	AddLine(startPos, endPos, 3, 0, 1000);

	startPos.x += SQUARE_SIZE;
	endPos.x -= SQUARE_SIZE;
	AddLine(startPos, endPos, 3, 0, 1000);
}

