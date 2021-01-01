/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "WeaponProjectileFactory.h"
#include "EmgProjectile.h"
#include "ExplosiveProjectile.h"
#include "FlameProjectile.h"
#include "FireBallProjectile.h"
#include "LaserProjectile.h"
#include "MissileProjectile.h"
#include "BeamLaserProjectile.h"
#include "LargeBeamLaserProjectile.h"
#include "LightningProjectile.h"
#include "StarburstProjectile.h"
#include "TorpedoProjectile.h"
#include "Sim/Misc/GlobalConstants.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Weapons/WeaponDef.h"

unsigned int WeaponProjectileFactory::LoadProjectile(const ProjectileParams& params) {
	const WeaponDef* weaponDef = params.weaponDef;
	const CWeaponProjectile* projectile = nullptr;

	assert(weaponDef != nullptr);

	switch (weaponDef->projectileType) {
		case WEAPON_EMG_PROJECTILE: {
			projectile = projMemPool.alloc<CEmgProjectile>(params);
		} break;

		case WEAPON_EXPLOSIVE_PROJECTILE: {
			projectile = projMemPool.alloc<CExplosiveProjectile>(params);
		} break;

		case WEAPON_FLAME_PROJECTILE: {
			projectile = projMemPool.alloc<CFlameProjectile>(params);
		} break;

		case WEAPON_FIREBALL_PROJECTILE: {
			projectile = projMemPool.alloc<CFireBallProjectile>(params);
		} break;

		case WEAPON_LASER_PROJECTILE: {
			projectile = projMemPool.alloc<CLaserProjectile>(params);
		} break;

		case WEAPON_MISSILE_PROJECTILE: {
			projectile = projMemPool.alloc<CMissileProjectile>(params);
		} break;

		case WEAPON_BEAMLASER_PROJECTILE: {
			projectile = projMemPool.alloc<CBeamLaserProjectile>(params);
		} break;

		case WEAPON_LARGEBEAMLASER_PROJECTILE: {
			projectile = projMemPool.alloc<CLargeBeamLaserProjectile>(params);
		} break;

		case WEAPON_LIGHTNING_PROJECTILE: {
			projectile = projMemPool.alloc<CLightningProjectile>(params);
		} break;

		case WEAPON_STARBURST_PROJECTILE: {
			projectile = projMemPool.alloc<CStarburstProjectile>(params);
		} break;

		case WEAPON_TORPEDO_PROJECTILE: {
			projectile = projMemPool.alloc<CTorpedoProjectile>(params);
		} break;

		default: {
			return -1u;
		} break;
	}

	return (projectile->id);
}

