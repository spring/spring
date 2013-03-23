/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "EmgProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/VertexArray.h"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Sync/SyncTracer.h"

CR_BIND_DERIVED(CEmgProjectile, CWeaponProjectile, (ProjectileParams()));

CR_REG_METADATA(CEmgProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(color),
	CR_RESERVED(8)
));

CEmgProjectile::CEmgProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	projectileType = WEAPON_EMG_PROJECTILE;

	if (weaponDef != NULL) {
		SetRadiusAndHeight(weaponDef->collisionSize, 0.0f);
		drawRadius = weaponDef->size;

		intensity = weaponDef->intensity;
		color = weaponDef->visuals.color;
	} else {
		intensity = 0.0f;
	}

#ifdef TRACE_SYNC
	tracefile << "New emg: ";
	tracefile << pos.x << " " << pos.y << " " << pos.z << " " << speed.x << " " << speed.y << " " << speed.z << "\n";
#endif
}

void CEmgProjectile::Update()
{
	if (!luaMoveCtrl) {
		pos += speed;
	}

	if (--ttl < 0) {
		if ((intensity = std::max(intensity - 0.1f, 0.0f)) <= 0.0f) {
			deleteMe = true;
		}
	} else {
		gCEG->Explosion(cegID, pos, ttl, intensity, NULL, 0.0f, NULL, speed);
	}
	UpdateGroundBounce();
	UpdateInterception();
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
