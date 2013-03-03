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
#include "Sim/Weapons/WeaponDef.h"

bool WeaponProjectileFactory::LoadProjectile(const ProjectileParams& params) {
	const WeaponDef* weaponDef = params.weaponDef;

	assert(weaponDef != NULL);

	switch (weaponDef->projectileType) {
		case WEAPON_EMG_PROJECTILE: {
			new CEmgProjectile(params, weaponDef->visuals.color, weaponDef->intensity);
		} break;

		case WEAPON_EXPLOSIVE_PROJECTILE: {
			new CExplosiveProjectile(params, weaponDef->damageAreaOfEffect, params.gravity);
		} break;

		case WEAPON_FLAME_PROJECTILE: {
			new CFlameProjectile(params, params.spread);
		} break;

		case WEAPON_FIREBALL_PROJECTILE: {
			new CFireBallProjectile(params);
		} break;

		case WEAPON_LASER_PROJECTILE: {
			new CLaserProjectile(params, weaponDef->duration * (weaponDef->projectilespeed * GAME_SPEED), weaponDef->visuals.color, weaponDef->visuals.color2, weaponDef->intensity);
		} break;

		case WEAPON_MISSILE_PROJECTILE: {
			new CMissileProjectile(params, weaponDef->damageAreaOfEffect, weaponDef->projectilespeed);
		} break;

		case WEAPON_BEAMLASER_PROJECTILE: {
			new CBeamLaserProjectile(params, params.startAlpha, params.endAlpha, weaponDef->visuals.color);
		} break;

		case WEAPON_LARGEBEAMLASER_PROJECTILE: {
			new CLargeBeamLaserProjectile(params, weaponDef->visuals.color, weaponDef->visuals.color2);
		} break;

		case WEAPON_LIGHTNING_PROJECTILE: {
			new CLightningProjectile(params, weaponDef->visuals.color);
		} break;

		case WEAPON_STARBURST_PROJECTILE: {
			new CStarburstProjectile(params, weaponDef->damageAreaOfEffect, weaponDef->projectilespeed, params.tracking, weaponDef->uptime * GAME_SPEED, params.maxRange, params.error);
		} break;

		case WEAPON_TORPEDO_PROJECTILE: {
			new CTorpedoProjectile(params, weaponDef->damageAreaOfEffect, weaponDef->projectilespeed, params.tracking);
		} break;

		default: {
			return false;
		} break;
	}

	return true;
}

