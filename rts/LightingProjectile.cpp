#include "StdAfx.h"
#include ".\lightingprojectile.h"
#include "projectilehandler.h"
#include "unit.h"
#include "mygl.h"
#include "camera.h"
#include "vertexarray.h"
#include "synctracer.h"
#include "weapon.h"
//#include "mmgr.h"

CLightingProjectile::CLightingProjectile(const float3& pos,const float3& end,CUnit* owner,const float3& color, int ttl,CWeapon* weap)
: CProjectile(pos,ZeroVector,owner),
	ttl(ttl),
	color(color),
	endPos(end),
	weapon(weap)
{
	checkCol=false;
	drawRadius=pos.distance(endPos);

	displacements[0]=0;
	for(int a=1;a<10;++a)
		displacements[a]=(gs->randFloat()-0.5)*drawRadius*0.05;

	displacements2[0]=0;
	for(int a=1;a<10;++a)
		displacements2[a]=(gs->randFloat()-0.5)*drawRadius*0.05;

	if(weapon)
		AddDeathDependence(weapon);

#ifdef TRACE_SYNC
	tracefile << "New lighting: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << end.x << " " << end.y << " " << end.z << "\n";
#endif
}

CLightingProjectile::~CLightingProjectile(void)
{
}

void CLightingProjectile::Update(void)
{
	ttl--;

	if(ttl<0){
		deleteMe=true;
	}

	if(weapon){
		pos=weapon->weaponPos;
	}
	for(int a=1;a<10;++a)
		displacements[a]+=(gs->randFloat()-0.5)*0.3;
	for(int a=1;a<10;++a)
		displacements2[a]+=(gs->randFloat()-0.5)*0.3;
}

void CLightingProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=color.x*255;
	col[1]=color.y*255;
	col[2]=color.z*255;
	col[3]=1;//intensity*255;

	float3 dir=(endPos-pos).Normalize();
	float3 dif(pos-camera->pos);
	float camDist=dif.Length();
	dif/=camDist;
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 tempPos=pos;

	for(int a=0;a<9;++a){
		float f=(a+1)*0.1;
		va->AddVertexTC(tempPos+dir1*(displacements[a]+0.8),11.0/16,0,    col);
		va->AddVertexTC(tempPos+dir1*(displacements[a]-0.8),11.0/16,1.0/8,col);
		tempPos=pos*(1-f)+endPos*f;
		va->AddVertexTC(tempPos+dir1*(displacements[a+1]-0.8),11.0/16,1.0/8,col);
		va->AddVertexTC(tempPos+dir1*(displacements[a+1]+0.8),11.0/16,0    ,col);
	}

	tempPos=pos;
	for(int a=0;a<9;++a){
		float f=(a+1)*0.1;
		va->AddVertexTC(tempPos+dir1*(displacements2[a]+0.8),11.0/16,0,    col);
		va->AddVertexTC(tempPos+dir1*(displacements2[a]-0.8),11.0/16,1.0/8,col);
		tempPos=pos*(1-f)+endPos*f;
		va->AddVertexTC(tempPos+dir1*(displacements2[a+1]-0.8),11.0/16,1.0/8,col);
		va->AddVertexTC(tempPos+dir1*(displacements2[a+1]+0.8),11.0/16,0    ,col);
	}
}

void CLightingProjectile::DependentDied(CObject* o)
{
	if(o==weapon)
		weapon=0;
	CProjectile::DependentDied(o);
}
