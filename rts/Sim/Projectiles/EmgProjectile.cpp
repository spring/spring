#include "StdAfx.h"
#include "EmgProjectile.h"
#include "ProjectileHandler.h"
#include "Sim/Units/Unit.h"
#include "Rendering/GL/myGL.h"
#include "Game/Camera.h"
#include "Rendering/GL/VertexArray.h"
#include "SyncTracer.h"
#include "mmgr.h"

CEmgProjectile::CEmgProjectile(const float3& pos,const float3& speed,CUnit* owner,const DamageArray& damages,const float3& color,float intensity, int ttl, WeaponDef *weaponDef)
: CWeaponProjectile(pos,speed,owner,0,float3(0,0,0), weaponDef,0),
	damages(damages),
	ttl(ttl),
	color(color),
	intensity(intensity)
{
	SetRadius(0.5);
	drawRadius=3;
#ifdef TRACE_SYNC
	tracefile << "New emg: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

CEmgProjectile::~CEmgProjectile(void)
{
}

void CEmgProjectile::Update(void)
{
	pos+=speed;
	ttl--;

	if(ttl<0){
		intensity-=0.1;
		if(intensity<=0){
			deleteMe=true;
			intensity=0;
		}
	}
}

void CEmgProjectile::Collision(CUnit* unit)
{
//	unit->DoDamage(damages,owner);

	CWeaponProjectile::Collision(unit);
}

void CEmgProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];
	col[0]=(unsigned char) (color.x*intensity*255);
	col[1]=(unsigned char) (color.y*intensity*255);
	col[2]=(unsigned char) (color.z*intensity*255);
	col[3]=5;//intensity*255;
	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*drawRadius-camera->up*drawRadius,0,0,col);
	va->AddVertexTC(interPos+camera->right*drawRadius-camera->up*drawRadius,0.125,0,col);
	va->AddVertexTC(interPos+camera->right*drawRadius+camera->up*drawRadius,0.125,0.125,col);
	va->AddVertexTC(interPos-camera->right*drawRadius+camera->up*drawRadius,0,0.125,col);
}
