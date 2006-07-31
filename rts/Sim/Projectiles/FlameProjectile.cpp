#include "StdAfx.h"
#include "FlameProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "mmgr.h"
#include "ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"

CFlameProjectile::CFlameProjectile(const float3& pos,const float3& speed,const float3& spread,CUnit* owner,const DamageArray& damages, WeaponDef *weaponDef, int ttl)
: CWeaponProjectile(pos,speed,owner,0,ZeroVector,weaponDef,damages,0),
	spread(spread),
	curTime(0)
{
	invttl=1.0/ttl;

	//SetRadius(speed.Length()*0.9);
	SetRadius(weaponDef->size);
}

CFlameProjectile::~CFlameProjectile(void)
{
}

void CFlameProjectile::Collision(void)
{
	float3 norm=ground->GetNormal(pos.x,pos.z);
	float ns=speed.dot(norm);
	speed-=norm*ns*1;
	pos.y+=0.05;
	curTime+=0.05;
}

void CFlameProjectile::Collision(CUnit* unit)
{
//	unit->DoDamage(damages*(1-curTime*curTime),owner);
	CWeaponProjectile::Collision(unit);
}

void CFlameProjectile::Update(void)
{
	pos+=speed;
	speed+=spread;

	SetRadius(radius+weaponDef->sizeGrowth);

	curTime+=invttl;
	if(curTime>1){
		curTime=1;
		deleteMe=true;
	}
}

void CFlameProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	if(curTime<0.33333){
		col[0]=(unsigned char) ((1-curTime)*255);
		col[1]=(unsigned char) ((1-curTime)*255);
		col[2]=(unsigned char) ((1-curTime*3)*(1-curTime)*255);
	} else if(curTime<0.66666){
		col[0]=(unsigned char) ((1-curTime)*255);
		col[1]=(unsigned char) ((1-(curTime-0.3333)*3)*(1-curTime)*255);
		col[2]=0;
	} else {
		col[0]=(unsigned char) ((1-(curTime-0.66666)*3)*(1-curTime)*255);
		col[1]=0;
		col[2]=0;
	}
	col[3]=1;//(0.3-curTime*0.3)*255;

	float3 interPos=pos+speed*gu->timeOffset;
		va->AddVertexTC(interPos-camera->right*radius-camera->up*radius,weaponDef->visuals.texture1->xstart ,weaponDef->visuals.texture1->ystart ,col);
		va->AddVertexTC(interPos+camera->right*radius-camera->up*radius,weaponDef->visuals.texture1->xend ,weaponDef->visuals.texture1->ystart ,col);
		va->AddVertexTC(interPos+camera->right*radius+camera->up*radius,weaponDef->visuals.texture1->xend ,weaponDef->visuals.texture1->yend ,col);
		va->AddVertexTC(interPos-camera->right*radius+camera->up*radius,weaponDef->visuals.texture1->xstart ,weaponDef->visuals.texture1->yend ,col);
}


