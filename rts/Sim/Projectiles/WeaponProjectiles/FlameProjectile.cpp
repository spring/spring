#include "StdAfx.h"
#include "mmgr.h"

#include "FlameProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "GlobalUnsynced.h"

CR_BIND_DERIVED(CFlameProjectile, CWeaponProjectile, (float3(0,0,0),float3(0,0,0),float3(0,0,0),NULL,NULL,0));

CR_REG_METADATA(CFlameProjectile,(
	CR_MEMBER(color),
	CR_MEMBER(color2),
	CR_MEMBER(intensity),
	CR_MEMBER(spread),
	CR_MEMBER(curTime),
	CR_MEMBER(physLife),
	CR_MEMBER(invttl),
	CR_RESERVED(16)
	));


CFlameProjectile::CFlameProjectile(const float3& pos, const float3& speed, const float3& spread,
		CUnit* owner, const WeaponDef* weaponDef, int ttl GML_PARG_C)
:	CWeaponProjectile(pos, speed, owner, 0, ZeroVector, weaponDef, 0, true,  ttl GML_PARG_P),
	color(color),
	color2(color2),
	intensity(intensity),
	spread(spread),
	curTime(0)
{
	invttl = 1.0f / ttl;
	if (weaponDef) {
		SetRadius(weaponDef->size*weaponDef->collisionSize);
		drawRadius = weaponDef->size;
		physLife = 1.0f / weaponDef->duration;
	}

	if (cegTag.size() > 0) {
		ceg.Load(explGenHandler, cegTag);
	}
}

CFlameProjectile::~CFlameProjectile(void)
{
}

void CFlameProjectile::Collision(void)
{
	if (ground->GetHeight2(pos.x, pos.z) < pos.y && weaponDef->waterweapon) {
		// prevent waterweapons from colliding with water
		return;
	}
	float3 norm = ground->GetNormal(pos.x, pos.z);
	float ns = speed.dot(norm);
	speed -= (norm * ns);
	pos.y += 0.05f;
	curTime += 0.05f;
}

void CFlameProjectile::Collision(CUnit* unit)
{
	CWeaponProjectile::Collision(unit);
}

void CFlameProjectile::Update(void)
{
	pos += speed;
	UpdateGroundBounce();
	speed += spread;

	radius = radius + weaponDef->sizeGrowth;
	sqRadius = radius * radius;
	drawRadius = radius * weaponDef->collisionSize;

	curTime += invttl;
	if (curTime > physLife)
		checkCol = false;
	if (curTime > 1) {
		curTime = 1;
		deleteMe = true;
	}

	if (cegTag.size() > 0) {
		ceg.Explosion(pos, curTime, intensity, 0x0, 0.0f, 0x0, speed);
	}
}

void CFlameProjectile::Draw(void)
{
	inArray = true;
	unsigned char col[4];
	weaponDef->visuals.colorMap->GetColor(col, curTime);

	va->AddVertexTC(drawPos - camera->right * radius - camera->up * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * radius - camera->up * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * radius + camera->up * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col);
	va->AddVertexTC(drawPos - camera->right * radius + camera->up * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col);
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
