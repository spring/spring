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
}

void CBombDropper::Init()
{
	CWeapon::Init();
	maxForwardAngleDif = -1.0f;
}


void CBombDropper::Update()
{
	if (targetType != Target_None) {
		weaponPos = owner->GetObjectSpacePos(relWeaponPos);

		if (targetType == Target_Unit) {
			// aim at base of unit instead of middle and ignore uncertainty
			targetPos = targetUnit->pos;
		}

		predict = GetPredictedImpactTime(targetPos);
	}

	CWeapon::Update();
}

bool CBombDropper::TestTarget(const float3& pos, bool userTarget, const CUnit* unit) const
{
	// assume we can still drop bombs on *partially* submerged targets
	if (!dropTorpedoes && TargetUnitOrPositionUnderWater(pos, unit))
		return false;
	// assume we can drop torpedoes on any partially or fully submerged target
	if (dropTorpedoes && !TargetUnitOrPositionInWater(pos, unit))
		return false;

	return CWeapon::TestTarget(pos, userTarget, unit);
}

bool CBombDropper::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	// TODO: requires sampling parabola from weaponPos down to dropPos
	return true;
}

#if 0
bool CBombDropper::CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const
{
	return (CWeapon::CanFire(true, ignoreTargetType, true));
}

bool CBombDropper::TestRange(const float3& pos, bool userTarget, const CUnit* unit) const
{
#else

bool CBombDropper::TestRange(const float3& pos, bool userTarget, const CUnit* unit) const { return true; }
bool CBombDropper::CanFire(bool ignoreAngleGood, bool ignoreTargetType, bool ignoreRequestedDir) const
{
	// we mostly ignore what AimWeapon has to say
	if (!CWeapon::CanFire(true, ignoreTargetType, true))
		return false;
#endif

	// bombs always fall down
	if (weaponPos.y <= targetPos.y)
		return false;

	// "normal" range restrictions are not meaningful
	// to check given dropped (ballistic) projectiles
	if (false && (owner->speed.w * predict) > weaponDef->range)
		return false;

	// dropPos is guaranteed to be in range from owner's
	// current position, now must make sure no bombs will
	// under- (eg. if range is much larger than <v*t>) or
	// overshoot (eg. if salvoDelay is long) their actual
	// targetPos too much
	// torpedoes especially should not be dropped if the
	// target position is already behind owner's position
	const float3 dropPos = owner->pos + owner->speed * predict;

	// salvoSize * salvoDelay is time to drop entire salvo
	// size*delay * speed is distance from dropPos of first
	// bomb to dropPos of last (assuming no spread), we use
	// half this distance as tolerance radius
	const float dropDist = std::max(1, salvoSize) * (salvoDelay * owner->speed.w * 0.5f);
	const float torpDist = torpMoveRange * (owner->frontdir.dot(targetPos - owner->pos) > 0.0f);

	return (dropPos.SqDistance2D(targetPos) < Square(dropDist + torpDist));
}

void CBombDropper::FireImpl(bool scriptCall)
{
	if (targetType == Target_Unit) {
		// aim at base of unit instead of middle and ignore uncertainity
		targetPos = targetUnit->pos;
	}

	if (dropTorpedoes) {
		float3 launchSpeed = owner->speed;

		// if owner is a hovering aircraft, use a fixed launching speed [?? WTF]
		if (owner->unitDef->IsHoveringAirUnit()) {
			launchSpeed = (targetPos - weaponPos).Normalize() * 5.0f;
		}

		ProjectileParams params = GetProjectileParams();
		params.pos = weaponPos;
		params.end = targetPos;
		params.speed = launchSpeed;
		params.ttl = (weaponDef->flighttime == 0)? ((range / projectileSpeed) + 15 + predict): weaponDef->flighttime;
		params.tracking = tracking;

		assert(weaponDef->projectileType == WEAPON_TORPEDO_PROJECTILE);
		WeaponProjectileFactory::LoadProjectile(params);
	} else {
		// fudge a bit better lateral aim to compensate for imprecise aircraft steering
		float3 dif = (targetPos - weaponPos) * XZVector;
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
		params.pos = weaponPos;
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
	CWeapon::SlowUpdate(true);
}

float CBombDropper::GetPredictedImpactTime(const float3& impactPos) const
{
	if (weaponPos.y <= impactPos.y)
		return 0.0f;

	// weapon needs <t> frames to drop a distance
	// <d> (if it has zero vertical speed), where:
	//   <d> = 0.5 * g * t*t = weaponPos.y - impactPos.y
	//   <t> = sqrt(d / (0.5 * g))
	// bombs will travel <v * t> elmos horizontally
	// which must be less than weapon's range to be
	// able to hit, otherwise will always overshoot
	const float d = impactPos.y - weaponPos.y;
	const float s = -owner->speed.y;

	const float g = mix(mapInfo->map.gravity, -weaponDef->myGravity, (weaponDef->myGravity != 0.0f));
	const float tt = (s - 2.0f * d) / -g;

	return ((tt >= 0.0f)? ((s / g) + math::sqrt(tt)): 0.0f);
}

