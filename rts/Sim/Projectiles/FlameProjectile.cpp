#include "StdAfx.h"
#include "FlameProjectile.h"
#include "Rendering/GL/VertexArray.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "mmgr.h"
#include "ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Rendering/Textures/ColorMap.h"

CFlameProjectile::CFlameProjectile(const float3& pos, const float3& speed, const float3& spread, CUnit* owner, const DamageArray& damages, WeaponDef *weaponDef, int ttl)
: CWeaponProjectile(pos,speed,owner,0,ZeroVector,weaponDef,damages,0),
	color(color),
	color2(color2),
	intensity(intensity),
	spread(spread),
	curTime(0)
{
	invttl=1.0f/ttl;
	SetRadius(weaponDef->size*weaponDef->collisionSize);
	drawRadius=weaponDef->size;
	physLife = 1.0f/weaponDef->duration;
}

CFlameProjectile::~CFlameProjectile(void)
{
}

void CFlameProjectile::Collision(void)
{
	float3 norm=ground->GetNormal(pos.x,pos.z);
	float ns=speed.dot(norm);
	speed-=norm*ns*1;
	pos.y+=0.05f;
	curTime+=0.05f;
}

void CFlameProjectile::Collision(CUnit* unit)
{
	CWeaponProjectile::Collision(unit);
}

void CFlameProjectile::Update(void)
{
	pos+=speed;
	speed+=spread;

	radius = radius+weaponDef->sizeGrowth;
	sqRadius = radius*radius;
	drawRadius = radius*weaponDef->collisionSize;

	curTime+=invttl;
	if(curTime>physLife)
		checkCol = false;
	if(curTime>1){
		curTime=1;
		deleteMe=true;
	}
}

void CFlameProjectile::Draw(void)
{
	inArray=true;
	unsigned char col[4];

	weaponDef->visuals.colorMap->GetColor(col, curTime);

	float3 interPos=pos+speed*gu->timeOffset;
	va->AddVertexTC(interPos-camera->right*radius-camera->up*radius,weaponDef->visuals.texture1->xstart ,weaponDef->visuals.texture1->ystart ,col);
	va->AddVertexTC(interPos+camera->right*radius-camera->up*radius,weaponDef->visuals.texture1->xend ,weaponDef->visuals.texture1->ystart ,col);
	va->AddVertexTC(interPos+camera->right*radius+camera->up*radius,weaponDef->visuals.texture1->xend ,weaponDef->visuals.texture1->yend ,col);
	va->AddVertexTC(interPos-camera->right*radius+camera->up*radius,weaponDef->visuals.texture1->xstart ,weaponDef->visuals.texture1->yend ,col);
}

int CFlameProjectile::ShieldRepulse(CPlasmaRepulser* shield,float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	float3 dir=pos-shieldPos;
	dir.Normalize();
	if(dir.dot(speed)<shieldMaxSpeed){
		speed+=dir*shieldForce;
		return 2;
	}
	return 0;
}
