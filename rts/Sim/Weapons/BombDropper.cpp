/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BombDropper.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/Team.h"
#include "Map/MapInfo.h"
#include "Sim/MoveTypes/HoverAirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/ExplosiveProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "System/myMath.h"
#include "System/mmgr.h"

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

	if (useTorps && owner) {
		owner->hasUWWeapons = true;
	}
}

CBombDropper::~CBombDropper()
{

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
				predict = s / ((weaponDef->myGravity == 0) ? mapInfo->map.gravity : -(weaponDef->myGravity)) + sqrt(sq);
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

bool CBombDropper::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (unit) {
		if (unit->isUnderWater && !dropTorpedoes) {
			return false;
		}
	} else {
		if (pos.y < 0) {
			return false;
		}
	}
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
			speed = targetPos - weaponPos;
			speed.Normalize();
			speed *= 5;
		}
		int ttl;
		if (weaponDef->flighttime == 0) {
			ttl = (int)((range / projectileSpeed) + 15 + predict);
		} else {
			ttl = weaponDef->flighttime;
		}
		new CTorpedoProjectile(weaponPos, speed, owner, damageAreaOfEffect,
				projectileSpeed, tracking, ttl, targetUnit, weaponDef);
	} else {
		// fudge a bit better lateral aim to compensate for imprecise aircraft steering
		float3 dif = targetPos-weaponPos;
		dif.y = 0;
		float3 dir = owner->speed;
		dir.SafeNormalize();
		// add a random spray
		dir += (gs->randVector() * sprayAngle+salvoError) * (1 - (owner->limExperience * 0.9f));
		dir.y = std::min(0.0f, dir.y);
		dir.SafeNormalize();
		dif -= dir * dif.dot(dir);
		dif /= std::max(0.01f,predict);
		const float size = dif.Length();
		if (size > 1.0f) {
			dif /= size * 1.0f;
		}
		new CExplosiveProjectile(weaponPos, owner->speed + dif, owner,
				weaponDef, 1000, damageAreaOfEffect,
				((weaponDef->myGravity == 0) ? mapInfo->map.gravity : -(weaponDef->myGravity)));
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

