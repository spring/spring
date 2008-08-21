#include "StdAfx.h"
#include "mmgr.h"

#include "creg/STL_Map.h"
#include "GeometricObjects.h"
#include "LogOutput.h"
#include "Map/ReadMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/GeoSquareProjectile.h"

CR_BIND(CGeometricObjects, );
CR_BIND(CGeometricObjects::GeoGroup, );

CR_REG_METADATA(CGeometricObjects, (
		CR_MEMBER(geoGroups),
		CR_MEMBER(toBeDeleted),
		CR_MEMBER(firstFreeGroup),
		CR_RESERVED(16)
		));

CR_REG_METADATA_SUB(CGeometricObjects, GeoGroup, (CR_MEMBER(squares)));


CGeometricObjects* geometricObjects;

CGeometricObjects::CGeometricObjects(void)
{
	firstFreeGroup = 1;
}


CGeometricObjects::~CGeometricObjects(void)
{
}

int CGeometricObjects::AddSpline(float3 b1, float3 b2, float3 b3, float3 b4, float width, int arrow, int lifeTime, int group)
{
	if (group == 0)
		group = firstFreeGroup++;

	float3 old1,old2;
	old1=CalcSpline( 0.00f,b1,b2,b3,b4);
	old2=CalcSpline( 0.05f,b1,b2,b3,b4);
	for(int a=0;a<20;++a){
		float3 np=CalcSpline(a*0.05f+0.1f,b1,b2,b3,b4);
		float3 dir1=(old2-old1).Normalize();
		float3 dir2=(np-old2).Normalize();

		if(arrow==1 && a==19){
			CGeoSquareProjectile* gsp=SAFE_NEW CGeoSquareProjectile(old1,old2,dir1,dir2,width,0);
			geoGroups[group].squares.push_back(gsp);
//			logOutput.Print("%f %f %f %f %f %f %f %f %f",old1.x,old1.y,old1.z,old2.x,old2.y,old2.z,np.x,np.y,np.z);
		} else {
			CGeoSquareProjectile* gsp=SAFE_NEW CGeoSquareProjectile(old1,old2,dir1,dir2,width*0.5f,width*0.5f);
			geoGroups[group].squares.push_back(gsp);
		}
		old1=old2;
		old2=np;
	}
	if(lifeTime>=0)
		toBeDeleted.insert(std::pair<int,int>(gs->frameNum+lifeTime,group));

	return group;
}


void CGeometricObjects::DeleteGroup(int group)
{
	GeoGroup* gg=&geoGroups[group];

	std::vector<CGeoSquareProjectile*>::iterator gi;

	for(gi=gg->squares.begin();gi!=gg->squares.end();++gi){
		(*gi)->deleteMe=true;
	}

	geoGroups.erase(group);
}


void CGeometricObjects::SetColor(int group, float r, float g, float b, float a)
{
	GeoGroup* gg=&geoGroups[group];

	std::vector<CGeoSquareProjectile*>::iterator gi;

	for(gi=gg->squares.begin();gi!=gg->squares.end();++gi){
		(*gi)->r=r;
		(*gi)->g=g;
		(*gi)->b=b;
		(*gi)->a=a;
	}
}


float3 CGeometricObjects::CalcSpline(float i, const float3& p1, const float3& p2, const float3& p3, const float3& p4)
{
	float ni=1-i;

	float3 res=p1*ni*ni*ni+p2*3*i*ni*ni+p3*3*i*i*ni+p4*i*i*i;
//	logOutput.Print("%f %f %f",res.x,res.y,res.z);
	return res;
}


int CGeometricObjects::AddLine(float3 start, float3 end, float width, int arrow, int lifetime, int group)
{
	if(group==0)
		group=firstFreeGroup++;

	float3 dir=(end-start).Normalize();
	if(arrow){
		CGeoSquareProjectile* gsp=SAFE_NEW CGeoSquareProjectile(start,start*0.2f+end*0.8f,dir,dir,width*0.5f,width*0.5f);
		geoGroups[group].squares.push_back(gsp);

		gsp=SAFE_NEW CGeoSquareProjectile(start*0.2f+end*0.8f,end,dir,dir,width,0);
		geoGroups[group].squares.push_back(gsp);

	} else {
		CGeoSquareProjectile* gsp=SAFE_NEW CGeoSquareProjectile(start,end,dir,dir,width*0.5f,width*0.5f);
		geoGroups[group].squares.push_back(gsp);
	}

	if(lifetime>=0)
		toBeDeleted.insert(std::pair<int,int>(gs->frameNum+lifetime,group));

	return group;
}


void CGeometricObjects::Update(void)
{
	while(!toBeDeleted.empty() && toBeDeleted.begin()->first<=gs->frameNum){
		DeleteGroup(toBeDeleted.begin()->second);
		toBeDeleted.erase(toBeDeleted.begin());
	}
}


void CGeometricObjects::MarkSquare(int mapSquare) {
	float3 startPos;
	startPos.x = int(mapSquare * SQUARE_SIZE) % gs->mapx;
	startPos.z = int(mapSquare * SQUARE_SIZE) / gs->mapx;
	startPos.y = readmap->centerheightmap[mapSquare];
	float3 endPos = startPos;
	endPos.x += SQUARE_SIZE;
	endPos.z += SQUARE_SIZE;
	AddLine(startPos, endPos, 3, 0, 1000);
	startPos.x += SQUARE_SIZE;
	endPos.x -= SQUARE_SIZE;
	AddLine(startPos, endPos, 3, 0, 1000);
}

