/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "mmgr.h"

#include "EmgProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Sync/SyncTracer.h"

CR_BIND_DERIVED(CEmgProjectile, CWeaponProjectile, (ZeroVector, ZeroVector, NULL, ZeroVector, 0, 0, NULL));

CR_REG_METADATA(CEmgProjectile,(
	CR_SETFLAG(CF_Synced),
    CR_MEMBER(intensity),
    CR_MEMBER(color),
    CR_RESERVED(8)
    ));

CEmgProjectile::CEmgProjectile(
	const float3& pos, const float3& speed,
	CUnit* owner,
	const float3& color, float intensity,
	int ttl, const WeaponDef* weaponDef):

	CWeaponProjectile(pos, speed, owner, NULL, ZeroVector, weaponDef, NULL, ttl),
	intensity(intensity),
	color(color)
{
	projectileType = WEAPON_EMG_PROJECTILE;

	if (weaponDef) {
		SetRadius(weaponDef->collisionSize);
		drawRadius = weaponDef->size;
	}
#ifdef TRACE_SYNC
	tracefile << "New emg: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif

	cegID = gCEG->Load(explGenHandler, cegTag);
}

CEmgProjectile::~CEmgProjectile()
{
}

void CEmgProjectile::Update()
{
	if (!luaMoveCtrl) {
		pos += speed;
	}

	if (--ttl < 0) {
		intensity -= 0.1f;
		if (intensity <= 0){
			deleteMe = true;
			intensity = 0;
		}
	} else {
		gCEG->Explosion(cegID, pos, ttl, intensity, NULL, 0.0f, NULL, speed);
	}
	UpdateGroundBounce();
}

void CEmgProjectile::Collision(CUnit* unit)
{
	CWeaponProjectile::Collision(unit);
}

void CEmgProjectile::Collision() {
	if (!(weaponDef->waterweapon && ground->GetHeightReal(pos.x, pos.z) < pos.y))
		CWeaponProjectile::Collision();
}

void CEmgProjectile::Draw()
{
	inArray = true;
	unsigned char col[4];
	col[0] = (unsigned char) (color.x * intensity * 255);
	col[1] = (unsigned char) (color.y * intensity * 255);
	col[2] = (unsigned char) (color.z * intensity * 255);
	col[3] = 5; //intensity*255;
	va->AddVertexTC(drawPos - camera->right * drawRadius-camera->up * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawRadius-camera->up * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col);
	va->AddVertexTC(drawPos + camera->right * drawRadius+camera->up * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col);
	va->AddVertexTC(drawPos - camera->right * drawRadius+camera->up * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col);
}

int CEmgProjectile::ShieldRepulse(CPlasmaRepulser* shield, float3 shieldPos, float shieldForce, float shieldMaxSpeed)
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
