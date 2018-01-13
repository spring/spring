/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "Cannon.h"
#include "WeaponDef.h"
#include "Game/TraceRay.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "Rendering/Env/Particles/Classes/HeatCloudProjectile.h"
#include "Rendering/Env/Particles/Classes/SmokeProjectile.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectileFactory.h"
#include "Sim/Units/Unit.h"
#include "System/Sync/SyncTracer.h"
#include "System/myMath.h"
#include "System/FastMath.h"

CR_BIND_DERIVED(CCannon, CWeapon, )

CR_REG_METADATA(CCannon,(
	CR_MEMBER(highTrajectory),
	CR_MEMBER(rangeFactor),
	CR_MEMBER(gravity),
	CR_MEMBER(lastTargetVec),
	CR_MEMBER(lastLaunchDir)
))

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CCannon::Init()
{
	gravity = mix(mapInfo->map.gravity, -weaponDef->myGravity, weaponDef->myGravity != 0.0f);
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
	wantedDir = GetWantedDir(currentTargetPos - aimFromPos);
}


bool CCannon::HaveFreeLineOfFire(const float3 srcPos, const float3 tgtPos, const SWeaponTarget& trg) const
{
	// assume we can still fire at partially submerged targets
	if (!weaponDef->waterweapon && TargetUnderWater(tgtPos, trg))
		return false;

	if (projectileSpeed == 0.0f)
		return true;

	float3 launchDir = CalcWantedDir(tgtPos - srcPos);
	float3 targetVec = (tgtPos - srcPos) * XZVector;

	if (launchDir.SqLength() == 0.0f)
		return false;
	if (targetVec.SqLength2D() == 0.0f)
		return true;

	// pick launchDir[0] if .x != 0, otherwise launchDir[2]
	const unsigned int dirIdx = 2 - 2 * (launchDir.x != 0.0f);

	const float xzTargetDist = targetVec.LengthNormalize();
	const float xzCoeffRatio = targetVec[dirIdx] / launchDir[dirIdx];

	// targetVec is normalized in the xz-plane while launchDir is xyz
	// therefore the linear parabolic coefficient has to be scaled by
	// their ratio or tested heights will fall short of those reached
	// by projectiles
	const float linCoeff = launchDir.y * xzCoeffRatio;
	const float qdrCoeff = (gravity * 0.5f) / (projectileSpeed * projectileSpeed);

	// CGround::SimTrajectoryGroundColDist(weaponMuzzlePos, launchDir, UpVector * gravity, {projectileSpeed, xzTargetDist - 10.0f})
	const float groundDist = ((avoidFlags & Collision::NOGROUND) == 0)?
		CGround::TrajectoryGroundCol(weaponMuzzlePos, targetVec, xzTargetDist - 10.0f, linCoeff, qdrCoeff):
		-1.0f;
	const float angleSpread = (AccuracyExperience() + SprayAngleExperience()) * 0.6f * 0.9f;

	if (groundDist > 0.0f)
		return false;

	// TODO: add a forcedUserTarget mode (enabled with meta key e.g.) and skip this test accordingly
	return (!TraceRay::TestTrajectoryCone(srcPos, targetVec, xzTargetDist, linCoeff, qdrCoeff, angleSpread, owner->allyteam, avoidFlags, owner));
}

void CCannon::FireImpl(const bool scriptCall)
{
	float3 targetVec = currentTargetPos - weaponMuzzlePos;
	float3 launchDir = (targetVec.SqLength() > 4.0f) ? GetWantedDir(targetVec) : targetVec; // prevent vertical aim when emit-sfx firing the weapon

	launchDir += (gsRNG.NextVector() * SprayAngleExperience() + SalvoErrorExperience());
	launchDir.SafeNormalize();

	int ttl = 0;
	const float sqSpeed2D = launchDir.SqLength2D() * projectileSpeed * projectileSpeed;
	const int predict = math::ceil((sqSpeed2D == 0.0f) ?
		(-2.0f * projectileSpeed * launchDir.y / gravity):
		math::sqrt(targetVec.SqLength2D() / sqSpeed2D));

	if (weaponDef->flighttime > 0) {
		ttl = weaponDef->flighttime;
	} else if (weaponDef->selfExplode) {
		ttl = (predict + gsRNG.NextFloat() * 2.5f - 0.5f);
	} else if ((weaponDef->groundBounce || weaponDef->waterBounce) && weaponDef->numBounce > 0) {
		ttl = (predict * (1 + weaponDef->numBounce * weaponDef->bounceRebound));
	} else {
		ttl = predict * 2;
	}

	ProjectileParams params = GetProjectileParams();
	params.pos = weaponMuzzlePos;
	params.end = currentTargetPos;
	params.speed = launchDir * projectileSpeed;
	params.ttl = ttl;
	params.gravity = gravity;

	WeaponProjectileFactory::LoadProjectile(params);
}

void CCannon::SlowUpdate()
{
	if (weaponDef->highTrajectory == 2 && owner->useHighTrajectory != highTrajectory)
		highTrajectory = owner->useHighTrajectory;

	CWeapon::SlowUpdate();
}


float3 CCannon::GetWantedDir(const float3& targetVec)
{
	const float3 tgtDif = targetVec - lastTargetVec;

	// try to cache results, sacrifice some (not much too much even for a pewee) accuracy
	// saves a dozen or two expensive calculations per second when 5 cannons are shooting
	// at several slow- and fast-moving targets
	if (math::fabs(tgtDif.x) < (SQUARE_SIZE / 4.0f) &&
		math::fabs(tgtDif.y) < (SQUARE_SIZE / 4.0f) &&
		math::fabs(tgtDif.z) < (SQUARE_SIZE / 4.0f)) {
		return lastLaunchDir;
	}

	const float3 launchDir = CalcWantedDir(targetVec);

	lastTargetVec = targetVec;
	lastLaunchDir = launchDir;
	return launchDir;
}

float3 CCannon::CalcWantedDir(const float3& targetVec) const
{
	const float Dsq = targetVec.SqLength();
	const float DFsq = targetVec.SqLength2D();
	const float g = gravity;
	const float v = projectileSpeed;
	const float dy  = targetVec.y;
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
		dir.x = targetVec.x;
		dir.z = targetVec.z;
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

	if (root1 < 0.0f)
		return 0.0f;

	return (rFact * (speed2dSq + speed2d * math::sqrt(root1)) / (-gravity));
}

float CCannon::GetRange2D(const float yDiff) const
{
	return (GetRange2D(yDiff, rangeFactor));
}

