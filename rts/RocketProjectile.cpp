// RocketProjectile.cpp: implementation of the CRocketProjectile class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RocketProjectile.h"
#include "SmokeProjectile.h"
#include "SmokeTrailProjectile.h"
#include "GameHelper.h"
#include "Unit.h"
#include "SyncTracer.h"
#include "VertexArray.h"
#include "myGL.h"
#include "Camera.h"
#include "InfoConsole.h"
#include "Ground.h"
#include "SyncTracer.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRocketProjectile::CRocketProjectile(const float3& pos,const float3& speed,CUnit* owner,DamageArray damages,float gravity,float wobble,float maxSpeed,const float3& targPos,float tracking, WeaponDef *weaponDef)
: CWeaponProjectile(pos,speed,owner,0,targPos,weaponDef,0),
	dir(speed),
	myGravity(gravity),
	wobble(wobble),
	maxSpeed(maxSpeed),
	damages(damages),
	oldSmoke(pos),
	age(0),
	targetPos(targPos),
	newWobble(0,0,0),
	oldWobble(0,0,0),
	wobbleUse(0),
	tracking(tracking)
{
	curSpeed=speed.Length();
	dir.Normalize();
	oldDir=dir;
#ifdef TRACE_SYNC
	tracefile << "New rocket: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
	drawRadius=8;
//	myGravity/=maxSpeed*maxSpeed;
}

CRocketProjectile::~CRocketProjectile()
{

}

void CRocketProjectile::Update()
{
	age++;
	wobbleUse+=1.0/16.0;
	curSpeed+=(maxSpeed-curSpeed)*0.02;
	dir+=oldWobble*(1-wobbleUse)+newWobble*wobbleUse;
	dir.Normalize();
	speed=dir*curSpeed;
	
	pos+=speed;
	if(!(age&7)){
		CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldDir,owner,false,false,1,90,0.6f);
//		tp->creationTime-=1;
		oldSmoke=pos;
		oldDir=dir;
	}
	if(!(age&15)){
		oldWobble=newWobble;
		wobbleUse=0;
		newWobble=gs->randVector()*wobble;
		newWobble.y+=myGravity;
		float3 wantedDir=targetPos-pos;
		float ydif=wantedDir.y;
		wantedDir.y=0;
		float dist=wantedDir.Length();
		wantedDir/=dist;
		float timeLeft=dist/curSpeed;
		wantedDir.y=(-myGravity*timeLeft*timeLeft*0.5+ydif)/(timeLeft);
		wantedDir.Normalize();
		float3 dif=wantedDir-dir;
		dif.Normalize();
		newWobble+=dif*tracking;

		if(pos.y-ground->GetApproximateHeight(pos.x,pos.z)>10)
			useAirLos=true;
	}
}

void CRocketProjectile::Collision()
{
//	helper->Explosion(pos,damages,damages[Armor_Other]*0.7,owner);
	CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldDir,owner,false,true,1,90,0.6f);
//	tp->creationTime-=1;
	
	CWeaponProjectile::Collision();
	oldSmoke=pos;
	oldDir=dir;
#ifdef TRACE_SYNC
	tracefile << "Rocket collision: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

void CRocketProjectile::Collision(CUnit *unit)
{
//	unit->DoDamage(damages,owner);
//	helper->Explosion(pos,damages,damages[Armor_Other]*0.7,owner);
	CSmokeTrailProjectile* tp=new CSmokeTrailProjectile(pos,oldSmoke,dir,oldDir,owner,false,true,1,90,0.6f);
//	tp->creationTime-=1;

	CWeaponProjectile::Collision(unit);
	oldSmoke=pos;
	oldDir=dir;
#ifdef TRACE_SYNC
	tracefile << "Rocket collision unit: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

void CRocketProjectile::Draw()
{
	float3 interPos=pos+speed*gu->timeOffset;
	inArray=true;
	float age2=(age&7)+gu->timeOffset;
	
	float3 dif(interPos-camera->pos);
	dif.Normalize();
	float3 dir1(dif.cross(dir));
	dir1.Normalize();
	float3 dif2(oldSmoke-camera->pos);
	dif2.Normalize();
	float3 dir2(dif2.cross(oldDir));
	dir2.Normalize();

	float color=0.6;

	unsigned char col[4];
	float a1=(1-float(0)/(90))*255;
	a1*=0.7+fabs(dif.dot(dir));
	float alpha=min(255.f,max(0.f,a1));
	col[0]=color*alpha;
	col[1]=color*alpha;
	col[2]=color*alpha;
	col[3]=alpha;

	unsigned char col2[4];
	float a2=(1-float(age2)/(90))*255;
	a2*=0.7+fabs(dif2.dot(oldDir));
	alpha=min(255.f,max(0.f,a2));
	col2[0]=color*alpha;
	col2[1]=color*alpha;
	col2[2]=color*alpha;
	col2[3]=alpha;

	float xmod=0;
	float ymod=0.25;
	float size=(0.2);
	float size2=(0.2+age2*(1/70.0));

	float txs=(1-age2/8.0);
	va->AddVertexTC(interPos-dir1*size, txs/4+1.0/32, 2.0/16, col2);
	va->AddVertexTC(interPos+dir1*size, txs/4+1.0/32, 3.0/16, col2);
	va->AddVertexTC(oldSmoke+dir2*size2, 0.25+1.0/32, 3.0/16, col2);
	va->AddVertexTC(oldSmoke-dir2*size2, 0.25+1.0/32, 2.0/16, col2);
	col[3]=255;
	float3 r=dir.cross(UpVector);
	r.Normalize();
	float3 u=dir.cross(r);
	va->AddVertexTC(interPos+r*0.1f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos-r*0.1f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir,1.0/16,1.0/16,col);

	va->AddVertexTC(interPos+u*0.1f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos-u*0.1f,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir,1.0/16,1.0/16,col);
	va->AddVertexTC(interPos+dir,1.0/16,1.0/16,col);
}
