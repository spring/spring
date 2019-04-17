/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */


#include "EmgProjectile.h"
#include "Game/Camera.h"
#include "Map/Ground.h"
#include "Rendering/GL/RenderDataBuffer.hpp"
#include "Rendering/Textures/TextureAtlas.h"
#include "Sim/Projectiles/ExplosionGenerator.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/Sync/SyncTracer.h"

CR_BIND_DERIVED(CEmgProjectile, CWeaponProjectile, )

CR_REG_METADATA(CEmgProjectile,(
	CR_SETFLAG(CF_Synced),
	CR_MEMBER(intensity),
	CR_MEMBER(color)
))


CEmgProjectile::CEmgProjectile(const ProjectileParams& params): CWeaponProjectile(params)
{
	projectileType = WEAPON_EMG_PROJECTILE;

	if (weaponDef != nullptr) {
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
	// disable collisions when ttl reaches 0 since the
	// projectile will travel far past its range while
	// fading out
	checkCol &= (ttl >= 0);
	deleteMe |= (intensity <= 0.0f);

	pos += (speed * (1 - luaMoveCtrl));

	if (ttl <= 0) {
		// fade out over the next 10 frames at most
		intensity -= 0.1f;
		intensity = std::max(intensity, 0.0f);
	} else {
		explGenHandler.GenExplosion(cegID, pos, speed, ttl, intensity, 0.0f, nullptr, nullptr);
	}

	UpdateGroundBounce();
	UpdateInterception();

	--ttl;
}

void CEmgProjectile::Draw(GL::RenderDataBufferTC* va) const
{
	unsigned char col[4];
	col[0] = (unsigned char) (color.x * intensity * 255);
	col[1] = (unsigned char) (color.y * intensity * 255);
	col[2] = (unsigned char) (color.z * intensity * 255);
	col[3] = intensity * 255;

	va->SafeAppend({drawPos - camera->GetRight() * drawRadius-camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col});
	va->SafeAppend({drawPos + camera->GetRight() * drawRadius-camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->ystart, col});
	va->SafeAppend({drawPos + camera->GetRight() * drawRadius+camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col});

	va->SafeAppend({drawPos + camera->GetRight() * drawRadius+camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xend,   weaponDef->visuals.texture1->yend,   col});
	va->SafeAppend({drawPos - camera->GetRight() * drawRadius+camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->yend,   col});
	va->SafeAppend({drawPos - camera->GetRight() * drawRadius-camera->GetUp() * drawRadius, weaponDef->visuals.texture1->xstart, weaponDef->visuals.texture1->ystart, col});
}

int CEmgProjectile::ShieldRepulse(const float3& shieldPos, float shieldForce, float shieldMaxSpeed)
{
	if (luaMoveCtrl)
		return 0;

	const float3 rdir = (pos - shieldPos).Normalize();

	if (rdir.dot(speed) < shieldMaxSpeed) {
		SetVelocityAndSpeed(speed + (rdir * shieldForce));
		return 2;
	}

	return 0;
}

