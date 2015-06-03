/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "BombDropper.h"
#include "WeaponDef.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/Team.h"
#include "Map/MapInfo.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "Sim/Units/UnitDef.h"
#include "System/myMath.h"
#include "System/Log/ILog.h"


CR_BIND_DERIVED(CBombDropper, CWeapon, (NULL, NULL, false))

CR_REG_METADATA(CBombDropper,(
	CR_MEMBER(dropTorpedoes),
	CR_MEMBER(torpMoveRange),
	CR_MEMBER(tracking),
	CR_RESERVED(16)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBombDropper::CBombDropper(CUnit* owner, const WeaponDef* def, bool useTorps)
	: CWeapon(owner, def)
	, dropTorpedoes(useTorps)
	, torpMoveRange(def->range * useTorps)
	, tracking((def->tracks)? def->turnrate: 0)
{
	onlyForward = true;
	doTargetGroundPos = true;
}

void CBombDropper::Init()
{
	CWeapon::Init();
	maxForwardAngleDif = -1.0f;
}

float CBombDropper::GetPredictFactor(float3 p) const
{
	return GetPredictedImpactTime(p - aimFromPos);
}

bool CBombDropper::TestTarget(const float3 pos, const SWeaponTarget& trg) const
{
	// assume we can still drop bombs on *partially* submerged targets
	if (!dropTorpedoes && TargetUnderWater(trg))
		return false;
	// assume we can drop torpedoes on any partially or fully submerged target
	if (dropTorpedoes && !TargetInWater(trg))
		return false;

	return CWeapon::TestTarget(pos, trg);
}

bool CBombDropper::HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg) const
{
	// TODO: requires sampling parabola from aimFromPos down to dropPos
	return true;
}


bool CBombDropper::TestRange(const float3 pos, const SWeaponTarget& trg) const
{
	// bombs always fall down
	if (aimFromPos.y < pos.y)
		return false;

	const float fallTime = GetPredictedImpactTime(pos - aimFromPos);
	const float dropDist = std::max(1, salvoSize) * salvoDelay * owner->speed.w * 0.5f;

	// torpedoes especially should not be dropped if the
	// target position is already behind owner's position
	const float torpDist = torpMoveRange * (owner->frontdir.dot(pos - aimFromPos) > 0.0f);

	return (pos.SqDistance2D(aimFromPos + owner->speed * fallTime) < Square(dropDist + torpDist));
}


bool CBombDropper::CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const
{
	return CWeapon::CanFire(true, ignoreTargetType, true);
}


void CBombDropper::FireImpl(const bool scriptCall)
{
	if (dropTorpedoes) {
		float3 launchSpeed = owner->speed;

		// if owner is a hovering aircraft, use a fixed launching speed [?? WTF]
		if (owner->unitDef->IsHoveringAirUnit()) {
			launchSpeed = wantedDir * 5.0f;
		}

		ProjectileParams params = GetProjectileParams();
		params.pos = weaponMuzzlePos;
		params.end = currentTargetPos;
		params.speed = launchSpeed;
		params.ttl = (weaponDef->flighttime == 0)? ((range / projectileSpeed) + 15 + predict): weaponDef->flighttime;
		params.tracking = tracking;

		assert(weaponDef->projectileType == WEAPON_TORPEDO_PROJECTILE);
		WeaponProjectileFactory::LoadProjectile(params);
	} else {
		// fudge a bit better lateral aim to compensate for imprecise aircraft steering
		float3 dif = (currentTargetPos - weaponMuzzlePos) * XZVector;
		float3 dir = owner->speed;

		dir = dir.SafeNormalize();
		// add a random spray
		dir += (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
		dir.y = std::min(0.0f, dir.y);
		dir = dir.SafeNormalize();

		dif -= (dir * dif.dot(dir));
		dif /= std::max(0.01f, predict);

		if (dif.SqLength() > 1.0f) {
			dif /= dif.Length();
		}

		ProjectileParams params = GetProjectileParams();
		params.pos = weaponMuzzlePos;
		params.speed = owner->speed + dif;
		params.ttl = 1000;
		params.gravity = (weaponDef->myGravity == 0)? mapInfo->map.gravity: -weaponDef->myGravity;

		assert(weaponDef->projectileType == WEAPON_EXPLOSIVE_PROJECTILE);
		WeaponProjectileFactory::LoadProjectile(params);
	}
}

/**
 * pass true for noAutoTargetOverride to make sure this weapon
 * does not generate its own targets (to save CPU mostly), only
 * allow user- or CAI-picked units (the latter for fight orders)
 */
void CBombDropper::SlowUpdate()
{
	CWeapon::SlowUpdate(true); //FIXME
}

float CBombDropper::GetPredictedImpactTime(const float3& impactPos) const
{
	if (weaponMuzzlePos.y <= impactPos.y)
		return 0.0f;

	// weapon needs <t> frames to drop a distance
	// <d> (if it has zero vertical speed), where:
	//   <d> = 0.5 * g * t*t = weaponMuzzlePos.y - impactPos.y
	//   <t> = sqrt(d / (0.5 * g))
	// bombs will travel <v * t> elmos horizontally
	// which must be less than weapon's range to be
	// able to hit, otherwise will always overshoot
	const float d = impactPos.y - weaponMuzzlePos.y;
	const float s = -owner->speed.y;

	const float g = (weaponDef->myGravity == 0) ? mapInfo->map.gravity: -weaponDef->myGravity;
	const float tt = (s - 2.0f * d) / -g;

	return ((tt >= 0.0f)? ((s / g) + math::sqrt(tt)): 0.0f);
}

