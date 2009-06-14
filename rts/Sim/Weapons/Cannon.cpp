// Cannon.cpp: implementation of the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Cannon.h"
#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "Map/MapInfo.h"
#include "myMath.h"
#include "Rendering/Env/BaseWater.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/ExplosiveProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sync/SyncTracer.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CCannon, CWeapon, (NULL));

CR_REG_METADATA(CCannon,(
	CR_MEMBER(maxPredict),
	CR_MEMBER(minPredict),
	CR_MEMBER(highTrajectory),
	CR_MEMBER(selfExplode),
	CR_MEMBER(rangeFactor),
	CR_MEMBER(lastDiff),
	CR_MEMBER(lastDir),
	CR_MEMBER(gravity),
	CR_RESERVED(32)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCannon::CCannon(CUnit* owner)
: CWeapon(owner),
	lastDiff(0.f, 0.f, 0.f),
	lastDir(0.f, -1.f, 0.f)
{
	highTrajectory=false;
	rangeFactor = 1;
}

void CCannon::Init(void)
{
	gravity = weaponDef->myGravity==0 ? mapInfo->map.gravity : -(weaponDef->myGravity);
	highTrajectory = weaponDef->highTrajectory == 1;
	if(highTrajectory){
		maxPredict=projectileSpeed*2/-gravity;
		minPredict=projectileSpeed*1.41f/-gravity;
	} else {
		maxPredict=projectileSpeed*1.41f/-gravity;
	}
	CWeapon::Init();

	// initialize range factor
	rangeFactor = 1;
	rangeFactor = (float)range/GetRange2D(0);
	// do not extend range if the modder specified speed too low
	// for the projectile to reach specified range
	if (rangeFactor > 1.f || rangeFactor <= 0.f)
		rangeFactor = 1.f;
	// some magical (but working) equations
	// useful properties: if rangeFactor == 1, heightBoostFactor == 1
	// TODO find something better?
	if (heightBoostFactor < 0.f)
		heightBoostFactor = (2.f - rangeFactor)/sqrt(rangeFactor);
}

CCannon::~CCannon()
{

}

void CCannon::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos + owner->frontdir*relWeaponPos.z
				 + owner->updir*relWeaponPos.y + owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos + owner->frontdir*relWeaponMuzzlePos.z
				 + owner->updir*relWeaponMuzzlePos.y + owner->rightdir*relWeaponMuzzlePos.x;
		float3 diff = targetPos-weaponPos;
		wantedDir = GetWantedDir(diff);
		float speed2D = wantedDir.Length2D() * projectileSpeed;
		predict = ((speed2D == 0) ? 1 : (diff.Length2D()/speed2D));
	} else {
		predict=0;
	}
	CWeapon::Update();
}

bool CCannon::TryTarget(const float3 &pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit)) {
		return false;
	}

	if (!weaponDef->waterweapon) {
		if (unit) {
			if (unit->isUnderWater) {
				return false;
			}
		} else {
			if (pos.y < 0) {
				return false;
			}
		}
	}

	if (projectileSpeed == 0) {
		return true;
	}

	float3 dif(pos - weaponMuzzlePos);
	float3 dir(GetWantedDir(dif));

	if (dir.SqLength() == 0) {
		return false;
	}

	float3 flatdir(dif.x, 0, dif.z);
	float flatlength = flatdir.Length();
	if (flatlength == 0) {
		return true;
	}
	flatdir /= flatlength;

	float gc = ground->TrajectoryGroundCol(weaponMuzzlePos, flatdir, flatlength - 10,
			dir.y , gravity / (projectileSpeed * projectileSpeed) * 0.5f);
	if (gc > 0) {
		return false;
	}

	float quadratic = gravity / (projectileSpeed * projectileSpeed) * 0.5f;
	float spread = (accuracy + sprayAngle) * 0.6f * (1 - owner->limExperience * 0.9f) * 0.9f;

	if (avoidFriendly && helper->TestTrajectoryAllyCone(weaponMuzzlePos, flatdir,
		flatlength - 30, dir.y, quadratic, spread, 3, owner->allyteam, owner)) {
		return false;
	}
	if (avoidNeutral && helper->TestTrajectoryNeutralCone(weaponMuzzlePos, flatdir,
		flatlength - 30, dir.y, quadratic, spread, 3, owner)) {
		return false;
	}

	return true;
}


void CCannon::FireImpl(void)
{
	float3 diff = targetPos-weaponMuzzlePos;
	float3 dir=(diff.SqLength() > 4.0) ? GetWantedDir(diff) : diff; //prevent vertical aim when emit-sfx firing the weapon
	dir+=(gs->randVector()*sprayAngle+salvoError)*(1-owner->limExperience*0.9f);
	dir.Normalize();

	int ttl = 0;
	float sqSpeed2D = dir.SqLength2D() * projectileSpeed * projectileSpeed;
	int predict = (int)ceil((sqSpeed2D == 0) ? (-2 * projectileSpeed * dir.y / gravity)
			: sqrt(diff.SqLength2D() / sqSpeed2D));
	if(weaponDef->flighttime > 0) {
		ttl = weaponDef->flighttime;
	} else if(selfExplode) {
		ttl = (int)(predict + gs->randFloat() * 2.5f - 0.5f);
	} else if((weaponDef->groundBounce || weaponDef->waterBounce)
			&& weaponDef->numBounce > 0) {
		ttl = (int)(predict * (1 + weaponDef->numBounce * weaponDef->bounceRebound));
	} else {
		ttl=predict*2;
	}

	new CExplosiveProjectile(weaponMuzzlePos, dir * projectileSpeed, owner,
		weaponDef, ttl, areaOfEffect, gravity);
}

void CCannon::SlowUpdate(void)
{
	if(weaponDef->highTrajectory == 2 && owner->useHighTrajectory!=highTrajectory){
		highTrajectory=owner->useHighTrajectory;
		if(highTrajectory){
			maxPredict=projectileSpeed*2/-gravity;
			minPredict=projectileSpeed*1.41f/-gravity;
		} else {
			maxPredict=projectileSpeed*1.41f/-gravity;
		}
	}
	CWeapon::SlowUpdate();
}

bool CCannon::AttackGround(float3 pos,bool userTarget)
{
	if(owner->directControl)		//mostly prevents firing longer than max range using fps mode
		pos.y=ground->GetHeight(pos.x,pos.z);

	return CWeapon::AttackGround(pos,userTarget);
}

float3 CCannon::GetWantedDir(const float3& diff)
{
	// try to cache results, sacrifice some (not much too much even for a pewee) accuracy
	// it saves a dozen or two expensive calculations per second when 5 guardians
	// are shooting at several slow- and fast-moving targets
	if (fabs(diff.x-lastDiff.x) < SQUARE_SIZE/4.f
			&& fabs(diff.y-lastDiff.y) < SQUARE_SIZE/4.f
			&& fabs(diff.z-lastDiff.z) < SQUARE_SIZE/4.f) {
		return lastDir;
	}

	float Dsq = diff.SqLength();
	float DFsq = diff.SqLength2D();
	float g = gravity;
	float v = projectileSpeed;
	float dy = diff.y;
	float dxz = sqrt(DFsq);
	float Vxz;
	float Vy;
	if(Dsq == 0) {
		Vxz = 0;
		Vy = highTrajectory ? v : -v;
	} else {
		float root1 = v*v*v*v + 2*v*v*g*dy-g*g*DFsq;
		if(root1 >= 0) {
			float root2 = 2*DFsq*Dsq*(v*v + g*dy + (highTrajectory ? -1 : 1)
				* sqrt(root1));
			if(root2 >= 0) {
				Vxz = sqrt(root2)/(2*Dsq);
				Vy = (dxz == 0 || Vxz == 0) ? v : (Vxz*dy/dxz - dxz*g/(2*Vxz));
			} else {
				Vxz = 0;
				Vy = 0;
			}
		} else {
			Vxz = 0;
			Vy = 0;
		}
	}
	float3 dir(diff.x, 0, diff.z);
	dir.Normalize();
	dir *= Vxz;
	dir.y = Vy;
	dir.Normalize();
	lastDiff = diff;
	lastDir = dir;
	return dir;
}

float CCannon::GetRange2D(float yDiff) const
{
	const float factor = 0.7071067f; // sin pi/4 == cos pi/4
	const float smoothHeight = 100.f;  // completely arbitrary
	const float speed2d = projectileSpeed*factor; // speed in one direction in max-range case
	const float speed2dSq = speed2d*speed2d;

	if (yDiff < -smoothHeight)
		yDiff *= heightBoostFactor;
	else if (yDiff < 0.f)
		// smooth a bit
		// f(0) == 1, f(smoothHeight) == heightBoostFactor
		yDiff *= 1.f + (heightBoostFactor-1.f) * (-yDiff)/smoothHeight;

	float root1 = speed2dSq + 2*gravity*yDiff;
	if(root1 < 0.f){
		return 0.f;
	} else {
		return rangeFactor*(speed2dSq + speed2d*sqrt(root1))/(-gravity);
	}
}
