/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BombDropper.h"
#include "WeaponDef.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/Team.h"
#include "Map/MapInfo.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CBombDropper, CWeapon, (NULL, false));

CR_REG_METADATA(CBombDropper,(
	CR_MEMBER(dropTorpedoes),
	CR_MEMBER(bombMoveRange),
	CR_MEMBER(tracking),
	CR_RESERVED(16)
));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBombDropper::CBombDropper(CUnit* owner, bool useTorps)
	: CWeapon(owner)
	, dropTorpedoes(useTorps)
	, bombMoveRange(3)
	, tracking(0)
{
	onlyForward = true;
}


void CBombDropper::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->pos
				+ (owner->frontdir * relWeaponPos.z)
				+ (owner->updir * relWeaponPos.y)
				+ (owner->rightdir * relWeaponPos.x);
		subClassReady = false;
		if (targetType == Target_Unit) {
			// aim at base of unit instead of middle and ignore uncertainity
			targetPos = targetUnit->pos;
		}
		if (weaponPos.y > targetPos.y) {
			const float d = targetPos.y - weaponPos.y;
			const float s = -owner->speed.y;
			const float sq = (s - 2*d) / -((weaponDef->myGravity == 0) ? mapInfo->map.gravity : -(weaponDef->myGravity));
			if (sq > 0) {
				predict = s / ((weaponDef->myGravity == 0) ? mapInfo->map.gravity : -(weaponDef->myGravity)) + math::sqrt(sq);
			} else {
				predict = 0;
			}
			const float3 hitpos = owner->pos+owner->speed*predict;
			const float speedf = owner->speed.Length();
			if (hitpos.SqDistance2D(targetPos) < Square((std::max(1, (salvoSize - 1)) * speedf * salvoDelay * 0.5f) + bombMoveRange))
			{
				subClassReady = true;
			}
		}
	}
	CWeapon::Update();
}

bool CBombDropper::TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const
{
	if (unit) {
		if (unit->IsUnderWater() && !dropTorpedoes) {
			return false;
		}
	} else {
		if (pos.y < 0) {
			return false;
		}
	}
	return CWeapon::TestTarget(pos, userTarget, unit);
}

bool CBombDropper::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	//TODO write me
	return true;
}

void CBombDropper::FireImpl()
{
	if (targetType == Target_Unit) {
		// aim at base of unit instead of middle and ignore uncertainity
		targetPos = targetUnit->pos;
	}

	if (dropTorpedoes) {
		float3 speed = owner->speed;

		if (dynamic_cast<CHoverAirMoveType*>(owner->moveType)) {
			speed = (targetPos - weaponPos).Normalize();
			speed *= 5.0f;
		}

		int ttl = weaponDef->flighttime;

		if (ttl == 0) {
			ttl = int((range / projectileSpeed) + 15 + predict);
		}

		ProjectileParams params = GetProjectileParams();
		params.pos = weaponPos;
		params.speed = speed;
		params.ttl = ttl;
		params.tracking = tracking;

		assert(weaponDef->projectileType == WEAPON_TORPEDO_PROJECTILE);
		WeaponProjectileFactory::LoadProjectile(params);
	} else {
		// fudge a bit better lateral aim to compensate for imprecise aircraft steering
		float3 dif = targetPos-weaponPos;
		dif.y = 0;
		float3 dir = owner->speed;
		dir.SafeNormalize();
		// add a random spray
		dir += (gs->randVector() * SprayAngleExperience()+SalvoErrorExperience());
		dir.y = std::min(0.0f, dir.y);
		dir.SafeNormalize();
		dif -= dir * dif.dot(dir);
		dif /= std::max(0.01f,predict);
		const float size = dif.Length();
		if (size > 1.0f) {
			dif /= size * 1.0f;
		}

		ProjectileParams params = GetProjectileParams();
		params.pos = weaponPos;
		params.speed = owner->speed + dif;
		params.ttl = 1000;
		params.gravity = (weaponDef->myGravity == 0)? mapInfo->map.gravity: -weaponDef->myGravity;

		assert(weaponDef->projectileType == WEAPON_EXPLOSIVE_PROJECTILE);
		WeaponProjectileFactory::LoadProjectile(params);
	}
}

void CBombDropper::Init()
{
	CWeapon::Init();
	maxForwardAngleDif = -1.0f;
}

/**
 * pass true for noAutoTargetOverride to make sure this weapon
 * does not generate its own targets (to save CPU mostly), only
 * allow user- or CAI-picked units (the latter for fight orders)
 */
void CBombDropper::SlowUpdate()
{
	CWeapon::SlowUpdate(true);
}

