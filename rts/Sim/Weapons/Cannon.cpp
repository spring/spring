/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Cannon.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "System/Sync/SyncTracer.h"
#include "WeaponDefHandler.h"
#include "System/myMath.h"
#include "System/FastMath.h"

CR_BIND_DERIVED(CCannon, CWeapon, (NULL, NULL))

CR_REG_METADATA(CCannon,(
	CR_MEMBER(highTrajectory),
	CR_MEMBER(rangeFactor),
	CR_MEMBER(lastDiff),
	CR_MEMBER(lastDir),
	CR_MEMBER(gravity)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCannon::CCannon(CUnit* owner, const WeaponDef* def)
	: CWeapon(owner, def)
	, rangeFactor(1.0f)
	, lastDir(-UpVector)
	, highTrajectory(false)
	, gravity(0.0f)
{
}

void CCannon::Init()
{
	gravity = (weaponDef->myGravity == 0)? mapInfo->map.gravity : -(weaponDef->myGravity);
	highTrajectory = (weaponDef->highTrajectory == 1);

	CWeapon::Init();
}

void CCannon::UpdateRange(const float val)
{
	// clamp so as to not extend range if projectile
	// speed is too low to reach the *updated* range
	// note: new range can be zero (!) making range
	// and height factors irrelevant
	range = val;
	rangeFactor = Clamp(range / GetRange2D(0.0f, 1.0f), 0.0f, 1.0f);

	// some magical (but working) equations
	// useful properties: if rangeFactor == 1, heightBoostFactor == 1
	// TODO find something better?
	if (heightBoostFactor < 0.0f && rangeFactor > 0.0f) {
		heightBoostFactor = (2.0f - rangeFactor) / math::sqrt(rangeFactor);
	}
}


void CCannon::UpdateWantedDir()
{
	const float3 targetVec = currentTargetPos - aimFromPos;
	wantedDir = GetWantedDir(targetVec);
}


bool CCannon::HaveFreeLineOfFire(const float3 pos, const SWeaponTarget& trg, bool useMuzzle) const
{
	// assume we can still fire at partially submerged targets
	if (!weaponDef->waterweapon && TargetUnderWater(pos, trg))
		return false;

	if (projectileSpeed == 0) {
		return true;
	}

	float3 dif(pos - weaponMuzzlePos);
	float3 dir(GetWantedDir2(dif));

	if (dir.SqLength() == 0) {
		return false;
	}

	float3 flatDir(dif.x, 0, dif.z);
	float flatLength = flatDir.Length();
	if (flatLength == 0) {
		return true;
	}
	flatDir /= flatLength;

	const float linear = dir.y;
	const float quadratic = gravity / (projectileSpeed * projectileSpeed) * 0.5f;
	const float groundDist = ((avoidFlags & Collision::NOGROUND) == 0)?
		CGround::TrajectoryGroundCol(weaponMuzzlePos, flatDir, flatLength - 10, linear, quadratic):
		-1.0f;
	const float spread = (AccuracyExperience() + SprayAngleExperience()) * 0.6f * 0.9f;

	if (groundDist > 0.0f) {
		return false;
	}

	//FIXME add a forcedUserTarget (a forced fire mode enabled with meta key or something) and skip the test below then
	if (TraceRay::TestTrajectoryCone(weaponMuzzlePos, flatDir, flatLength,
		dir.y, quadratic, spread, owner->allyteam, avoidFlags, owner)) {
		return false;
	}

	return true;
}

void CCannon::FireImpl(const bool scriptCall)
{
	float3 diff = currentTargetPos - weaponMuzzlePos;
	float3 dir = (diff.SqLength() > 4.0f) ? GetWantedDir(diff) : diff; // prevent vertical aim when emit-sfx firing the weapon

	dir += (gs->randVector() * SprayAngleExperience() + SalvoErrorExperience());
	dir.SafeNormalize();

	int ttl = 0;
	const float sqSpeed2D = dir.SqLength2D() * projectileSpeed * projectileSpeed;
	const int predict = math::ceil((sqSpeed2D == 0.0f) ?
		(-2.0f * projectileSpeed * dir.y / gravity):
		math::sqrt(diff.SqLength2D() / sqSpeed2D));

	if (weaponDef->flighttime > 0) {
		ttl = weaponDef->flighttime;
	} else if (weaponDef->selfExplode) {
		ttl = (predict + gs->randFloat() * 2.5f - 0.5f);
	} else if ((weaponDef->groundBounce || weaponDef->waterBounce) && weaponDef->numBounce > 0) {
		ttl = (predict * (1 + weaponDef->numBounce * weaponDef->bounceRebound));
	} else {
		ttl = predict * 2;
	}

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.speed = dir * projectileSpeed;
	params.ttl = ttl;
	params.gravity = gravity;

	WeaponProjectileFactory::LoadProjectile(params);
}

void CCannon::SlowUpdate()
{
	if (weaponDef->highTrajectory == 2 && owner->useHighTrajectory != highTrajectory) {
		highTrajectory = owner->useHighTrajectory;
	}

	CWeapon::SlowUpdate();
}


float3 CCannon::GetWantedDir(const float3& diff)
{
	// try to cache results, sacrifice some (not much too much even for a pewee) accuracy
	// it saves a dozen or two expensive calculations per second when 5 guardians
	// are shooting at several slow- and fast-moving targets
	if (math::fabs(diff.x - lastDiff.x) < (SQUARE_SIZE / 4.0f) &&
		math::fabs(diff.y - lastDiff.y) < (SQUARE_SIZE / 4.0f) &&
		math::fabs(diff.z - lastDiff.z) < (SQUARE_SIZE / 4.0f)) {
		return lastDir;
	}

	const float3 dir = GetWantedDir2(diff);
	lastDiff = diff;
	lastDir  = dir;
	return dir;
}

float3 CCannon::GetWantedDir2(const float3& diff) const
{
	const float Dsq = diff.SqLength();
	const float DFsq = diff.SqLength2D();
	const float g = gravity;
	const float v = projectileSpeed;
	const float dy  = diff.y;
	const float dxz = math::sqrt(DFsq);
	float Vxz = 0.0f;
	float Vy  = 0.0f;

	if (Dsq == 0.0f) {
		Vy = highTrajectory ? v : -v;
	} else {
		// FIXME: temporary safeguards against FP overflow
		// (introduced by extreme off-map unit positions; the term
		// DFsq * Dsq * ... * dy should never even approach 1e38)
		if (Dsq < 1e12f && math::fabs(dy) < 1e6f) {
			const float root1 = v*v*v*v + 2.0f*v*v*g*dy - g*g*DFsq;

			if (root1 >= 0.0f) {
				const float root2 = 2.0f * DFsq * Dsq * (v * v + g * dy + (highTrajectory ? -1.0f : 1.0f) * math::sqrt(root1));

				if (root2 >= 0.0f) {
					Vxz = math::sqrt(root2) / (2.0f * Dsq);
					Vy = (dxz == 0.0f || Vxz == 0.0f) ? v : (Vxz * dy / dxz  -  dxz * g / (2.0f * Vxz));
				}
			}
		}
	}

	float3 dir = ZeroVector;

	if (Vxz != 0.0f || Vy != 0.0f) {
		dir.x = diff.x;
		dir.z = diff.z;
		dir.SafeNormalize();
		dir *= Vxz;
		dir.y = Vy;
		dir.SafeNormalize();
	}

	return dir;
}

float CCannon::GetRange2D(float yDiff, float rFact) const
{
	const float speedFactor = 0.7071067f; // sin pi/4 == cos pi/4
	const float smoothHeight = 100.0f;  // completely arbitrary
	const float speed2d = projectileSpeed * speedFactor; // speed in one direction in max-range case
	const float speed2dSq = speed2d * speed2d;

	if (yDiff < -smoothHeight) {
		yDiff *= heightBoostFactor;
	} else if (yDiff < 0.0f) {
		// smooth a bit
		// f(0) == 1, f(smoothHeight) == heightBoostFactor
		yDiff *= 1.0f + (heightBoostFactor - 1.0f) * (-yDiff)/smoothHeight;
	}

	const float root1 = speed2dSq + 2.0f * gravity * yDiff;

	if (root1 < 0.0f) {
		return 0.0f;
	}

	return (rFact * (speed2dSq + speed2d * math::sqrt(root1)) / (-gravity));
}

float CCannon::GetRange2D(const float yDiff) const
{
	return (GetRange2D(yDiff, rangeFactor));
}

