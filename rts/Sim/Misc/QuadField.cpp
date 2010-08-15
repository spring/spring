/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "QuadField.h"

#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Features/Feature.h"
#include "Sim/Units/Unit.h"
#include "System/creg/STL_List.h"

CR_BIND(CQuadField, );
CR_REG_METADATA(CQuadField, (
//		CR_MEMBER(baseQuads),
		CR_MEMBER(numQuadsX),
		CR_MEMBER(numQuadsZ),
//		CR_MEMBER(tempQuads),
		CR_RESERVED(8),
		CR_SERIALIZER(Serialize)
		));


CQuadField::Quad::Quad() : teamUnits(teamHandler->ActiveAllyTeams())
{
};

void CQuadField::Serialize(creg::ISerializer& s)
{
	// no need to alloc quad array, this has already been done in constructor
	for (int z = 0; z < numQuadsZ; ++z)
		for (int x = 0; x < numQuadsX; ++x)
			s.SerializeObjectInstance(&baseQuads[z*numQuadsX+x], Quad::StaticClass());
}



CR_BIND(CQuadField::Quad, );

CR_REG_METADATA_SUB(CQuadField, Quad, (
		CR_MEMBER(units),
		CR_MEMBER(teamUnits),
		CR_MEMBER(features)
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


vector<int> CQuadField::GetQuads(float3 pos,float radius) const
{
	pos.CheckInBounds();

	vector<int> ret;
	int maxx = std::min(((int)(pos.x + radius)) / QUAD_SIZE + 1, numQuadsX - 1);
	int maxz = std::min(((int)(pos.z + radius)) / QUAD_SIZE + 1, numQuadsZ - 1);

	int minx = std::max(((int)(pos.x - radius)) / QUAD_SIZE, 0);
	int minz = std::max(((int)(pos.z - radius)) / QUAD_SIZE, 0);

	if (maxz < minz || maxx < minx)
		return ret;

	float maxSqLength = (radius + QUAD_SIZE * 0.72f) * (radius + QUAD_SIZE * 0.72f);
	ret.reserve((maxz - minz) * (maxx - minx));
	for (int z = minz; z <= maxz; ++z)
		for (int x = minx; x <= maxx; ++x)
			if ((pos - float3(x * QUAD_SIZE + QUAD_SIZE * 0.5f, 0, z * QUAD_SIZE + QUAD_SIZE * 0.5f)).SqLength2D() < maxSqLength)
				ret.push_back(z * numQuadsX + x);

	return ret;
}


void CQuadField::GetQuads(float3 pos,float radius, int*& dst) const
{
	pos.CheckInBounds();

	int maxx = std::min(((int)(pos.x + radius)) / QUAD_SIZE + 1, numQuadsX - 1);
	int maxz = std::min(((int)(pos.z + radius)) / QUAD_SIZE + 1, numQuadsZ - 1);

	int minx = std::max(((int)(pos.x - radius)) / QUAD_SIZE, 0);
	int minz = std::max(((int)(pos.z - radius)) / QUAD_SIZE, 0);

	if (maxz < minz || maxx < minx)
		return;

	float maxSqLength = (radius + QUAD_SIZE * 0.72f) * (radius + QUAD_SIZE * 0.72f);
	for (int z = minz; z <= maxz; ++z)
		for (int x = minx; x <= maxx; ++x)
			if ((pos - float3(x * QUAD_SIZE + QUAD_SIZE * 0.5f, 0, z * QUAD_SIZE + QUAD_SIZE * 0.5f)).SqLength2D() < maxSqLength) {
				*dst = z * numQuadsX + x;
				++dst;
			}
}

/**
 * Returns all units within @c radius of @c pos,
 * and treats each unit as a 3D point object
 */
std::vector<CUnit*> CQuadField::GetUnits(const float3& pos, float radius)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnits

	std::vector<CUnit*> units;

	int* endQuad = tempQuads;
	const int tempNum = gs->tempNum++;

	GetQuads(pos, radius, endQuad);

	for (int* a = tempQuads; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (std::list<CUnit*>::iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum != tempNum) {
				(*ui)->tempNum = tempNum;
				units.push_back(*ui);
			}
		}
	}

	return units;
}

/**
 * Returns all units within @c radius of @c pos,
 * and takes the 3D model radius of each unit into account.
 */
std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& pos, float radius, bool spherical)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnitsExact

	std::vector<CUnit*> units;

	int* endQuad = tempQuads;
	const int tempNum = gs->tempNum++;

	GetQuads(pos, radius, endQuad);

	for (int* a = tempQuads; a != endQuad; ++a) {
		Quad& quad = baseQuads[*a];

		for (std::list<CUnit*>::iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if ((*ui)->tempNum != tempNum) {
				const float
					totRad   = radius + (*ui)->radius,
					totRadSq = totRad * totRad;
				const float posUnitDstSq = spherical?
					(pos - (*ui)->midPos).SqLength():
					(pos - (*ui)->midPos).SqLength2D();

				if (posUnitDstSq < totRadSq) {
					(*ui)->tempNum = tempNum;
					units.push_back(*ui);
				}
			}
		}
	}

	return units;
}

//! returns all units within the rectangle defined by
//! mins and maxs, which extends infnitely along the
//! y-axis
std::vector<CUnit*> CQuadField::GetUnitsExact(const float3& mins, const float3& maxs)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnitsExact

	std::vector<CUnit*> units;
	std::vector<int> quads = GetQuadsRectangle(mins, maxs);

	int tempNum = gs->tempNum++;

	std::vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CUnit*>& quadUnits = baseQuads[*qi].units;
		std::list<CUnit*>::iterator ui;
		for (ui = quadUnits.begin(); ui != quadUnits.end(); ++ui) {
			CUnit* unit = *ui;
			const float3& pos = unit->midPos;
			if ((unit->tempNum != tempNum) &&
			    (pos.x > mins.x) && (pos.x < maxs.x) &&
			    (pos.z > mins.z) && (pos.z < maxs.z)) {
				unit->tempNum = tempNum;
				units.push_back(unit);
			}
		}
	}

	return units;
}

std::vector<int> CQuadField::GetQuadsOnRay(const float3& start, float3 dir, float length)
{
	int* end = tempQuads;
	GetQuadsOnRay(start,dir,length,end);

	return std::vector<int>(tempQuads, end);
}

void CQuadField::GetQuadsOnRay(float3 start, float3 dir,float length, int*& dst)
{
	if(start.x<1){
		if(dir.x==0)
			dir.x=0.00001f;
		start=start+dir*((1-start.x)/dir.x);
	}
	if(start.x>gs->mapx*SQUARE_SIZE-1){
		if(dir.x==0)
			dir.x=0.00001f;
		start=start+dir*((gs->mapx*SQUARE_SIZE-1-start.x)/dir.x);
	}
	if(start.z<1){
		if(dir.z==0)
			dir.z=0.00001f;
		start=start+dir*((1-start.z)/dir.z);
	}
	if(start.z>gs->mapy*SQUARE_SIZE-1){
		if(dir.z==0)
			dir.z=0.00001f;
		start=start+dir*((gs->mapy*SQUARE_SIZE-1-start.z)/dir.z);
	}

	if(start.x<1){
		start.x=1;
	}
	if(start.x>gs->mapx*SQUARE_SIZE-1){
		start.x=gs->mapx*SQUARE_SIZE-1;
	}
	if(start.z<1){
		start.z=1;
	}
	if(start.z>gs->mapy*SQUARE_SIZE-1){
		start.z=gs->mapy*SQUARE_SIZE-1;
	}

	float3 to=start+dir*length;

	if(to.x<1){
		to=to-dir*((to.x-1)/dir.x);
	}
	if(to.x>gs->mapx*SQUARE_SIZE-1){
		to=to-dir*((to.x-gs->mapx*SQUARE_SIZE+1)/dir.x);
	}
	if(to.z<1){
		to=to-dir*((to.z-1)/dir.z);
	}
	if(to.z>gs->mapy*SQUARE_SIZE-1){
		to=to-dir*((to.z-gs->mapy*SQUARE_SIZE+1)/dir.z);
	}
	//these 4 shouldnt be needed but sometimes we seem to get strange enough values that rounding errors throw us outide the map
	if(to.x<1){
		to.x=1;
	}
	if(to.x>gs->mapx*SQUARE_SIZE-1){
		to.x=gs->mapx*SQUARE_SIZE-1;
	}
	if(to.z<1){
		to.z=1;
	}
	if(to.z>gs->mapy*SQUARE_SIZE-1){
		to.z=gs->mapy*SQUARE_SIZE-1;
	}

	float dx=to.x-start.x;
	float dz=to.z-start.z;
	float xp=start.x;
	float zp=start.z;
	float invQuadSize=1.0f/QUAD_SIZE;

	if((floor(start.x*invQuadSize)==floor(to.x*invQuadSize)) && (floor(start.z*invQuadSize)==floor(to.z*invQuadSize))){
		*dst=((int(start.x*invQuadSize))+(int(start.z*invQuadSize))*numQuadsX);
		++dst;
	} else if(floor(start.x*invQuadSize)==floor(to.x*invQuadSize)){
		int first=(int(start.x*invQuadSize))+(int(start.z*invQuadSize))*numQuadsX;
		int last=(int(to.x*invQuadSize))+(int(to.z*invQuadSize))*numQuadsX;
		if(dz>0)
			for(int a=first;a<=last;a+=numQuadsX){
				*dst=(a);
				++dst;
			}
		else
			for(int a=first;a>=last;a-=numQuadsX){
				*dst=(a);
				++dst;
			}
	} else if(floor(start.z*invQuadSize)==floor(to.z*invQuadSize)){
		int first=(int(start.x*invQuadSize))+(int(start.z*invQuadSize))*numQuadsX;
		int last=(int(to.x*invQuadSize))+(int(to.z*invQuadSize))*numQuadsX;
		if(dx>0)
			for(int a=first;a<=last;a++){
				*dst=(a);
				++dst;
			}
		else
			for(int a=first;a>=last;a--){
				*dst=(a);
				++dst;
			}
	} else {
		float xn,zn;
		bool keepgoing=true;
		for(int i = 0; i < 1000 && keepgoing; i++){
			*dst=((int(zp*invQuadSize))*numQuadsX+(int(xp*invQuadSize)));
			++dst;

			if(dx>0){
				xn=(floor(xp*invQuadSize)*QUAD_SIZE+QUAD_SIZE-xp)/dx;
			} else {
				xn=(floor(xp*invQuadSize)*QUAD_SIZE-xp)/dx;
			}
			if(dz>0){
				zn=(floor(zp*invQuadSize)*QUAD_SIZE+QUAD_SIZE-zp)/dz;
			} else {
				zn=(floor(zp*invQuadSize)*QUAD_SIZE-zp)/dz;
			}

			if(xn<zn){
				xp+=(xn+0.0001f)*dx;
				zp+=(xn+0.0001f)*dz;
			} else {
				xp+=(zn+0.0001f)*dx;
				zp+=(zn+0.0001f)*dz;
			}
			keepgoing=fabs(xp-start.x)<fabs(to.x-start.x) && fabs(zp-start.z)<fabs(to.z-start.z);
		}
	}
}

void CQuadField::MovedUnit(CUnit *unit)
{
	vector<int> newQuads=GetQuads(unit->pos,unit->radius);

	//! compare if the quads have changed, if not stop here
	if(newQuads.size()==unit->quads.size()){
		vector<int>::iterator qi1,qi2;
		qi1=unit->quads.begin();
		for(qi2=newQuads.begin();qi2!=newQuads.end();++qi2){
			if(*qi1!=*qi2)
				break;
			++qi1;
		}
		if(qi2==newQuads.end())
			return;
	}

	GML_RECMUTEX_LOCK(quad); // MovedUnit - possible performance hog

	std::vector<int>::iterator qi;
	for (qi = unit->quads.begin(); qi != unit->quads.end(); ++qi) {
		std::list<CUnit*>::iterator ui;
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			if(*ui==unit){
				baseQuads[*qi].units.erase(ui);
				break;
			}
		}
		for(ui=baseQuads[*qi].teamUnits[unit->allyteam].begin();ui!=baseQuads[*qi].teamUnits[unit->allyteam].end();++ui){
			if(*ui==unit){
				baseQuads[*qi].teamUnits[unit->allyteam].erase(ui);
				break;
			}
		}
	}
	for(qi=newQuads.begin();qi!=newQuads.end();++qi){
		baseQuads[*qi].units.push_front(unit);
		baseQuads[*qi].teamUnits[unit->allyteam].push_front(unit);
	}
	unit->quads=newQuads;
}

void CQuadField::RemoveUnit(CUnit* unit)
{
	GML_RECMUTEX_LOCK(quad); // RemoveUnit

	std::vector<int>::iterator qi;
	for (qi = unit->quads.begin(); qi != unit->quads.end(); ++qi) {
		std::list<CUnit*>::iterator ui;
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			if(*ui==unit){
				baseQuads[*qi].units.erase(ui);
				break;
			}
		}
		for(ui=baseQuads[*qi].teamUnits[unit->allyteam].begin();ui!=baseQuads[*qi].teamUnits[unit->allyteam].end();++ui){
			if(*ui==unit){
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

	vector<int> newQuads=GetQuads(feature->pos,feature->radius);

	vector<int>::iterator qi;
	for(qi=newQuads.begin();qi!=newQuads.end();++qi){
		baseQuads[*qi].features.push_front(feature);
	}
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	GML_RECMUTEX_LOCK(quad); // RemoveFeature

	vector<int> quads=GetQuads(feature->pos,feature->radius);

	std::vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		baseQuads[*qi].features.remove(feature);
	}
}

vector<CFeature*> CQuadField::GetFeaturesExact(const float3& pos,float radius)
{
	GML_RECMUTEX_LOCK(qnum); // GetFeaturesExact

	vector<CFeature*> features;
	vector<int> quads=GetQuads(pos,radius);

	int tempNum=gs->tempNum++;

	std::vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CFeature*>::iterator fi;
		for (fi = baseQuads[*qi].features.begin(); fi != baseQuads[*qi].features.end(); ++fi) {
			float totRad=radius+(*fi)->radius;
			if((*fi)->tempNum!=tempNum && (pos-(*fi)->midPos).SqLength()<totRad*totRad){
				(*fi)->tempNum=tempNum;
				features.push_back(*fi);
			}
		}
	}

	return features;
}

std::vector<CFeature*> CQuadField::GetFeaturesExact(const float3& mins, const float3& maxs)
{
	GML_RECMUTEX_LOCK(qnum); // GetFeaturesExact

	std::vector<CFeature*> features;
	std::vector<int> quads = GetQuadsRectangle(mins, maxs);

	int tempNum = gs->tempNum++;

	std::vector<int>::iterator qi;
	for(qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CFeature*>& quadFeatures = baseQuads[*qi].features;
		std::list<CFeature*>::iterator fi;
		for (fi = quadFeatures.begin(); fi != quadFeatures.end(); ++fi) {
			CFeature* feature = *fi;
			const float3& pos = feature->midPos;
			if ((feature->tempNum != tempNum) &&
				  (pos.x > mins.x) && (pos.x < maxs.x) &&
					(pos.z > mins.z) && (pos.z < maxs.z)) {
				feature->tempNum = tempNum;
				features.push_back(feature);
			}
		}
	}

	return features;
}

std::vector<CSolidObject*> CQuadField::GetSolidsExact(const float3& pos,float radius)
{
	GML_RECMUTEX_LOCK(qnum); // GetSolidsExact

	std::vector<CSolidObject*> solids;
	std::vector<int> quads = GetQuads(pos,radius);
	int tempNum = gs->tempNum++;

	std::vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		std::list<CUnit*>::iterator ui;
		for (ui = baseQuads[*qi].units.begin(); ui != baseQuads[*qi].units.end(); ++ui) {
			if (!(*ui)->blocking)
				continue;

			float totRad=radius+(*ui)->radius;
			if((*ui)->tempNum!=tempNum && (pos-(*ui)->midPos).SqLength()<totRad*totRad){
				(*ui)->tempNum=tempNum;
				solids.push_back(*ui);
			}
		}

		std::list<CFeature*>::iterator fi;
		for(fi=baseQuads[*qi].features.begin();fi!=baseQuads[*qi].features.end();++fi){
			if (!(*fi)->blocking)
				continue;

			float totRad=radius+(*fi)->radius;
			if((*fi)->tempNum!=tempNum && (pos-(*fi)->midPos).SqLength()<totRad*totRad){
				(*fi)->tempNum=tempNum;
				solids.push_back(*fi);
			}
		}
	}

	return solids;
}

std::vector<int> CQuadField::GetQuadsRectangle(const float3& pos,const float3& pos2) const
{
	std::vector<int> ret;

	int maxx = std::max(0, std::min(((int)(pos2.x)) / QUAD_SIZE + 1, numQuadsX - 1));
	int maxz = std::max(0, std::min(((int)(pos2.z)) / QUAD_SIZE + 1, numQuadsZ - 1));

	int minx = std::max(0, std::min(((int)(pos.x)) / QUAD_SIZE, numQuadsX - 1));
	int minz = std::max(0, std::min(((int)(pos.z)) / QUAD_SIZE, numQuadsZ - 1));

	if (maxz < minz || maxx < minx)
		return ret;

	ret.reserve((maxz - minz) * (maxx - minx));
	for(int z = minz; z <= maxz; ++z)
		for(int x = minx; x <= maxx; ++x)
			ret.push_back(z * numQuadsX + x);

	return ret;
}

// optimization specifically for projectile collisions
void CQuadField::GetUnitsAndFeaturesExact(const float3& pos, float radius, CUnit**& dstUnit, CFeature**& dstFeature)
{
	GML_RECMUTEX_LOCK(qnum); // GetUnitsAndFeaturesExact

	int tempNum=gs->tempNum++;

	int* endQuad = tempQuads;
	GetQuads(pos, radius, endQuad);

	for(int* a=tempQuads;a!=endQuad;++a){
		Quad& quad = baseQuads[*a];
		for (std::list<CUnit*>::iterator ui = quad.units.begin(); ui != quad.units.end(); ++ui) {
			if((*ui)->tempNum!=tempNum){
				(*ui)->tempNum=tempNum;
				*dstUnit=(*ui);
				++dstUnit;
			}
		}

		for (std::list<CFeature*>::iterator fi = quad.features.begin(); fi != quad.features.end(); ++fi) {
			float totRad=radius+(*fi)->radius;
			if((*fi)->tempNum!=tempNum && (pos-(*fi)->midPos).SqLength()<totRad*totRad){
				(*fi)->tempNum=tempNum;
				*dstFeature=(*fi);
				++dstFeature;
			}
		}
	}
}

