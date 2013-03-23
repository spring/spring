/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "FlameProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/ColorMap.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"

CR_BIND_DERIVED(CFlameProjectile, CWeaponProjectile, (ProjectileParams()));

CR_REG_METADATA(CFlameProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(spread),
	CR_MEMBER(curTime),
	CR_MEMBER(physLife),
	CR_MEMBER(invttl),
	CR_RESERVED(16)
));


CFlameProjectile::CFlameProjectile(const ProjectileParams& params): CWeaponProjectile(params)
	, curTime(0.0f)
	, physLife(0.0f)
	, invttl(0.0f)
{
	projectileType = WEAPON_FLAME_PROJECTILE;

	invttl = 1.0f / ttl;
	spread = params.spread;

	if (weaponDef != NULL) {
		SetRadiusAndHeight(weaponDef->size * weaponDef->collisionSize, 0.0f);
		drawRadius = weaponDef->size;

		physLife = 1.0f / weaponDef->duration;
	}
}

void CFlameProjectile::Collision()
{
	const float3 norm = ground->GetNormal(pos.x, pos.z);
	const float ns = speed.dot(norm);

	speed -= (norm * ns);
	pos.y += 0.05f;
	curTime += 0.05f;
}

void CFlameProjectile::Update()
{
	if (!luaMoveCtrl) {
		pos += speed;
		UpdateGroundBounce();
		speed += spread;
	}
	UpdateInterception();

	radius = radius + weaponDef->sizeGrowth;
	sqRadius = radius * radius;
	drawRadius = radius * weaponDef->collisionSize;

	curTime += invttl;
	if (curTime > physLife) {
		checkCol = false;
	}
	if (curTime > 1) {
		curTime = 1;
		deleteMe = true;
	}

	gCEG->Explosion(cegID, pos, curTime, 0.0f, NULL, 0.0f, NULL, speed);
}

void CFlameProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	weaponDef->visuals.colorMap->GetColor(col, curTime);

	va->AddVertexTC(drawPos - camera->right * radius - camera->up * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * radius - camera->up * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * radius + camera->up * radius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col);
	va->AddVertexTC(drawPos - camera->right * radius + camera->up * radius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col);
}

int CFlameProjectile::ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (!luaMoveCtrl) {
		const float3 rdir = (pos - shieldPos).Normalize();

		if (rdir.dot(speed) < shieldMaxSpeed) {
			speed += (rdir * shieldForce);
			return 2;
		}
	}

	return 0;
}
