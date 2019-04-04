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
#include "System/SpringMath.h"
#include "System/Log/ILog.h"


CR_BIND_DERIVED(CBombDropper, CWeapon, )

CR_REG_METADATA(CBombDropper,(
	CR_MEMBER(dropTorpedoes),
	CR_MEMBER(torpMoveRange),
	CR_MEMBER(tracking)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CBombDropper::CBombDropper(CUnit* owner, const WeaponDef* def, bool useTorps)
	: CWeapon(owner, def)
	, dropTorpedoes(useTorps)
{
	// null happens when loading
	if (def != nullptr) {
		torpMoveRange = def->range * useTorps;
		tracking = def->turnrate * def->tracks;
	}

	onlyForward = true;
	doTargetGroundPos = true;
	noAutoTarget = true;
	maxForwardAngleDif = -1.0f;
}


float CBombDropper::GetPredictedImpactTime(float3 impactPos) const
{
	if (weaponMuzzlePos.y <= impactPos.y)
		return 0.0f;

	// Definitions:
	// d := (mostly negative) distance from plane to explosion position
	// v := vertical plane speed (bomb starts with it)
	// g := myGravity (negative)
	//
	// We want to solve:    d = v * t + 0.5 * g * t^2
	//                  <=> 0 = t^2 + (2v/g) * t - (2d/g)
	//                             -> a=2v/g    b=-(2d/g)
	// Using pq-formula there are 2 solutions (we want the t>0 +solution):
	//                 x(+,-) = -p +- sqrt((p^2 - q)  with p=a/2   q=b
	//             =>         = -v/g + sqrt(v^2/g^2 + 2d/g)
	//            <=>         = -v/g + sqrt( (v^2 + 2dg)/g^2 )
	//            <=>         = -v/g + sqrt(v^2 + 2dg) / |g|
	// Now we use (g is negative!):  +1/|g| = +1/(-g) = -1/g
	//             =>         = (-v - sqrt(v^2 + 2dg)) / g
	//            <=>         = -(v + sqrt(v^2 + 2dg)) / g

	const float d = impactPos.y - weaponMuzzlePos.y;
	const float v = owner->speed.y;
	const float g = (weaponDef->myGravity == 0) ? mapInfo->map.gravity: -weaponDef->myGravity;

	const float tt = v*v + 2.f * d * g;

	return ((tt >= 0.0f)? ((-v - math::sqrt(tt)) / g) : 0.0f);
}


bool CBombDropper::TestTarget(const float3 pos, const SWeaponTarget& trg) const
{
	// assume we can still drop bombs on *partially* submerged targets
	if (!dropTorpedoes && TargetUnderWater(pos, trg))
		return false;
	// assume we can drop torpedoes on any partially or fully submerged target
	if (dropTorpedoes && !TargetInWater(pos, trg))
		return false;

	return CWeapon::TestTarget(pos, trg);
}

bool CBombDropper::TestRange(const float3 pos, const SWeaponTarget& trg) const
{
	// bombs always fall down
	if (aimFromPos.y < pos.y)
		return false;

	const float fallTime = GetPredictedImpactTime(pos);
	const float dropDist = std::max(1, salvoSize - 1) * salvoDelay * owner->speed.Length2D() * 0.5f;

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
	const float predict = GetPredictedImpactTime(currentTargetPos);

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
		dir += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
		dir.y = std::min(0.0f, dir.y);
		dir = dir.SafeNormalize();

		dif -= (dir * dif.dot(dir));
		dif /= std::max(0.01f, predict);

		if (dif.SqLength() > 1.0f) {
			dif /= dif.Length();
		}

		ProjectileParams params = GetProjectileParams();
		params.pos = weaponMuzzlePos;
		params.end = currentTargetPos;
		params.speed = owner->speed + dif;
		params.ttl = 1000;
		params.gravity = (weaponDef->myGravity == 0)? mapInfo->map.gravity: -weaponDef->myGravity;

		assert(weaponDef->projectileType == WEAPON_EXPLOSIVE_PROJECTILE);
		WeaponProjectileFactory::LoadProjectile(params);
	}
}

