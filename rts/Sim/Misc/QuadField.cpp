#include "StdAfx.h"
// QuadField.cpp: implementation of the CQuadField class.
//
//////////////////////////////////////////////////////////////////////

#include "QuadField.h"

#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitHandler.h"
#include "LogOutput.h"
#include "Feature.h"
#include "creg/STL_List.h"
#include "mmgr.h"

CR_BIND(CQuadField, );
CR_REG_METADATA(CQuadField, (
				CR_MEMBER(numQuadsX), CR_MEMBER(numQuadsZ), CR_SERIALIZER(creg_Serialize)));

void CQuadField::creg_Serialize(creg::ISerializer& s)
{
	// no need to alloc quad array, this has already been done in constructor
	for(int y=0;y<numQuadsZ;++y)
		for(int x=0;x<numQuadsX;++x)
			s.SerializeObjectInstance(&baseQuads[y*numQuadsX+x], Quad::StaticClass());
}

CR_BIND(CQuadField::Quad, );

CR_REG_METADATA_SUB(CQuadField, Quad, (
		//float startx;  // constant, assigned in constructor
		//float starty;
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
	numQuadsX=gs->mapx*SQUARE_SIZE/QUAD_SIZE;
	numQuadsZ=gs->mapy*SQUARE_SIZE/QUAD_SIZE;

	baseQuads=SAFE_NEW Quad[numQuadsX*numQuadsZ];

	for(int y=0;y<numQuadsZ;++y){
		for(int x=0;x<numQuadsX;++x){
			baseQuads[y*numQuadsX+x].startx=x*QUAD_SIZE;
			baseQuads[y*numQuadsX+x].starty=y*QUAD_SIZE;
		}
	}

	tempQuads = new int[1000];
	tempUnitsArray = new (CUnit(*[MAX_UNITS]));
	tempFeaturesArray = new (CFeature(*[MAX_UNITS]));
}

CQuadField::~CQuadField()
{
	delete[] baseQuads;
	delete[] tempQuads;
	delete[] tempUnitsArray;
	delete[] tempFeaturesArray;
}

vector<int> CQuadField::GetQuads(float3 pos,float radius)
{
	pos.CheckInBounds();

	vector<int> ret;
	int maxx=min(((int)(pos.x+radius))/QUAD_SIZE+1,numQuadsX-1);
	int maxy=min(((int)(pos.z+radius))/QUAD_SIZE+1,numQuadsZ-1);

	int minx=max(((int)(pos.x-radius))/QUAD_SIZE,0);
	int miny=max(((int)(pos.z-radius))/QUAD_SIZE,0);
	
	if(maxy<miny || maxx<minx)
		return ret;

	float maxSqLength=(radius+QUAD_SIZE*0.72f)*(radius+QUAD_SIZE*0.72f);
	ret.reserve((maxy-miny)*(maxx-minx));
	for(int y=miny;y<=maxy;++y)
		for(int x=minx;x<=maxx;++x)
			if((pos-float3(x*QUAD_SIZE+QUAD_SIZE*0.5f,0,y*QUAD_SIZE+QUAD_SIZE*0.5f)).SqLength2D()<maxSqLength)
				ret.push_back(y*numQuadsX+x);

	return ret;
}


void CQuadField::GetQuads(float3 pos,float radius, int*& dst)
{
	pos.CheckInBounds();

	int maxx=min(((int)(pos.x+radius))/QUAD_SIZE+1,numQuadsX-1);
	int maxy=min(((int)(pos.z+radius))/QUAD_SIZE+1,numQuadsZ-1);

	int minx=max(((int)(pos.x-radius))/QUAD_SIZE,0);
	int miny=max(((int)(pos.z-radius))/QUAD_SIZE,0);
	
	if(maxy<miny || maxx<minx)
		return;

	int* org = dst;
	float maxSqLength=(radius+QUAD_SIZE*0.72f)*(radius+QUAD_SIZE*0.72f);
	for(int y=miny;y<=maxy;++y)
		for(int x=minx;x<=maxx;++x)
			if((pos-float3(x*QUAD_SIZE+QUAD_SIZE*0.5f,0,y*QUAD_SIZE+QUAD_SIZE*0.5f)).SqLength2D()<maxSqLength)
			{
				*dst = y*numQuadsX+x;
				++dst;
			}
}


void CQuadField::MovedUnit(CUnit *unit)
{
	vector<int> newQuads=GetQuads(unit->pos,unit->radius);

	if(newQuads.size()==unit->quads.size()){
		bool same=true;
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
	vector<int>::iterator qi;
	for(qi=unit->quads.begin();qi!=unit->quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=baseQuads[*qi].units.begin();ui!=baseQuads[*qi].units.end();++ui){
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

vector<CUnit*> CQuadField::GetUnits(const float3& pos,float radius)
{
	vector<CUnit*> units;

	int* endQuad=tempQuads;
	GetQuads(pos,radius,endQuad);
	
	int tempNum=gs->tempNum++;

	for(int* a=tempQuads;a!=endQuad;++a){
		Quad& quad = baseQuads[*a];
		for(list<CUnit*>::iterator ui=quad.units.begin();ui!=quad.units.end();++ui){
			if((*ui)->tempNum!=tempNum){
				(*ui)->tempNum=tempNum;
				units.push_back(*ui);
			}
		}
	}

	return units;
}

vector<CUnit*> CQuadField::GetUnitsExact(const float3& pos,float radius)
{
	vector<CUnit*> units;
/*	if(pos.x<0 || pos.z<0 || pos.x>gs->mapx*SQUARE_SIZE || pos.z>gs->mapy*SQUARE_SIZE){
		logOutput.Print("Trying to get units outside map %.0f %.0f",pos.x,pos.z);
		return units;
	}*/

	int* endQuad=tempQuads;
	GetQuads(pos,radius,endQuad);
	
	int tempNum=gs->tempNum++;

	for(int* a=tempQuads;a!=endQuad;++a){
		Quad& quad = baseQuads[*a];
		for(list<CUnit*>::iterator ui=quad.units.begin();ui!=quad.units.end();++ui){
			float totRad=radius+(*ui)->radius;
			if((*ui)->tempNum!=tempNum && (pos-(*ui)->midPos).SqLength()<totRad*totRad){
				(*ui)->tempNum=tempNum;
				units.push_back(*ui);
			}
		}
	}

	return units;
}

vector<CUnit*> CQuadField::GetUnitsExact(const float3& mins, const float3& maxs)
{
	vector<CUnit*> units;

	vector<int> quads = GetQuadsRectangle(mins, maxs);
	
	int tempNum = gs->tempNum++;

	vector<int>::iterator qi;
	for (qi = quads.begin(); qi != quads.end(); ++qi) {
		list<CUnit*>& quadUnits = baseQuads[*qi].units;
		list<CUnit*>::iterator ui;
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

vector<int> CQuadField::GetQuadsOnRay(const float3& start, float3 dir, float length)
{

	int* end = tempQuads;
	GetQuadsOnRay(start,dir,length,end);

	return vector<int>(tempQuads,end);
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
//	if(to.x<0){
///		logOutput.Print("error %f %f %f %f %f %f %f %f",start.x,start.z,to.x,to.z,dir.x,dir.z,dir.y,length);
//	}

	float dx=to.x-start.x;
	float dz=to.z-start.z;
	float xp=start.x;
	float zp=start.z;
	float xn,zn;
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

void CQuadField::RemoveUnit(CUnit* unit)
{
	std::vector<int>::iterator qi;
	for(qi=unit->quads.begin();qi!=unit->quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=baseQuads[*qi].units.begin();ui!=baseQuads[*qi].units.end();++ui){
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
}

void CQuadField::AddFeature(CFeature* feature)
{
	vector<int> newQuads=GetQuads(feature->pos,feature->radius);

	vector<int>::iterator qi;
	for(qi=newQuads.begin();qi!=newQuads.end();++qi){
		baseQuads[*qi].features.push_front(feature);
	}
//	feature->addPos=feature->pos;
//	feature->addRadius=feature->radius;/**/
}

void CQuadField::RemoveFeature(CFeature* feature)
{
	vector<int> quads=GetQuads(feature->pos,feature->radius);

	std::vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		baseQuads[*qi].features.remove(feature);
	}/*
	for(int a=0;a<numQuadsX*numQuadsZ;++a){
		for(std::list<CFeature*>::iterator fi=baseQuads[a].features.begin();fi!=baseQuads[a].features.end();++fi){
			if(*fi==feature){
				baseQuads[a].features.erase(fi);
				logOutput.Print("Feature not removed correctly");
				logOutput.Print("Pos %.0f %.0f Radius %.0f",feature->pos.x,feature->pos.z,feature->radius);
				logOutput.Print("Pos %.0f %.0f Radius %.0f",feature->addPos.x,feature->addPos.z,feature->addRadius);
				for(qi=quads.begin();qi!=quads.end();++qi)
					logOutput << *qi << " ";
				logOutput << "\n";
				logOutput.Print("New %i",a);
				break;
			}
		}
	}/**/
}

vector<CFeature*> CQuadField::GetFeaturesExact(const float3& pos,float radius)
{
	vector<CFeature*> features;
/*	if(pos.x<0 || pos.z<0 || pos.x>gs->mapx*SQUARE_SIZE || pos.z>gs->mapy*SQUARE_SIZE){
		logOutput.Print("Trying to get units outside map %.0f %.0f",pos.x,pos.z);
		return units;
	}*/

	vector<int> quads=GetQuads(pos,radius);
	
	int tempNum=gs->tempNum++;

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CFeature*>::iterator fi;
		for(fi=baseQuads[*qi].features.begin();fi!=baseQuads[*qi].features.end();++fi){
			float totRad=radius+(*fi)->radius;
			if((*fi)->tempNum!=tempNum && (pos-(*fi)->midPos).SqLength()<totRad*totRad){
				(*fi)->tempNum=tempNum;
				features.push_back(*fi);
			}
		}
	}

	return features;
}

vector<CFeature*> CQuadField::GetFeaturesExact(const float3& mins,
                                               const float3& maxs)
{
	vector<CFeature*> features;

	vector<int> quads = GetQuadsRectangle(mins, maxs);
	
	int tempNum = gs->tempNum++;

	vector<int>::iterator qi;
	for(qi = quads.begin(); qi != quads.end(); ++qi) {
		list<CFeature*>& quadFeatures = baseQuads[*qi].features;
		list<CFeature*>::iterator fi;
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

vector<CSolidObject*> CQuadField::GetSolidsExact(const float3& pos,float radius)
{
	vector<CSolidObject*> solids;
/*	if(pos.x<0 || pos.z<0 || pos.x>gs->mapx*SQUARE_SIZE || pos.z>gs->mapy*SQUARE_SIZE){
		logOutput.Print("Trying to get units outside map %.0f %.0f",pos.x,pos.z);
		return units;
	}*/

	vector<int> quads=GetQuads(pos,radius);
	
	int tempNum=gs->tempNum++;

	vector<int>::iterator qi;
	for(qi=quads.begin();qi!=quads.end();++qi){
		list<CUnit*>::iterator ui;
		for(ui=baseQuads[*qi].units.begin();ui!=baseQuads[*qi].units.end();++ui){
			if (!(*ui)->blocking)
				continue;

			float totRad=radius+(*ui)->radius;
			if((*ui)->tempNum!=tempNum && (pos-(*ui)->midPos).SqLength()<totRad*totRad){
				(*ui)->tempNum=tempNum;
				solids.push_back(*ui);
			}
		}

		list<CFeature*>::iterator fi;
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

vector<int> CQuadField::GetQuadsRectangle(const float3& pos,const float3& pos2)
{
	vector<int> ret;

	int maxx=max(0,min(((int)(pos2.x))/QUAD_SIZE+1,numQuadsX-1));
	int maxy=max(0,min(((int)(pos2.z))/QUAD_SIZE+1,numQuadsZ-1));

	int minx=max(0,min(((int)(pos.x))/QUAD_SIZE,numQuadsX-1));
	int miny=max(0,min(((int)(pos.z))/QUAD_SIZE,numQuadsZ-1));
	
	if(maxy<miny || maxx<minx)
		return ret;

	ret.reserve((maxy-miny)*(maxx-minx));
	for(int y=miny;y<=maxy;++y)
		for(int x=minx;x<=maxx;++x)
			ret.push_back(y*numQuadsX+x);

	return ret;

}

// optimization specifically for projectile collisions
void CQuadField::GetUnitsAndFeaturesExact(const float3& pos, float radius, CUnit**& dstUnit, CFeature**& dstFeature)
{
	int tempNum=gs->tempNum++;

	int* endQuad = tempQuads;
	GetQuads(pos, radius, endQuad);

	for(int* a=tempQuads;a!=endQuad;++a){
		Quad& quad = baseQuads[*a];
		for(list<CUnit*>::iterator ui=quad.units.begin();ui!=quad.units.end();++ui){
			if((*ui)->tempNum!=tempNum){
				(*ui)->tempNum=tempNum;
				*dstUnit=(*ui);
				++dstUnit;
			}
		}

		for(list<CFeature*>::iterator fi=quad.features.begin();fi!=quad.features.end();++fi){
			float totRad=radius+(*fi)->radius;
			if((*fi)->tempNum!=tempNum && (pos-(*fi)->midPos).SqLength()<totRad*totRad){
				(*fi)->tempNum=tempNum;
				*dstFeature=(*fi);
				++dstFeature;
			}
		}
	}
}

