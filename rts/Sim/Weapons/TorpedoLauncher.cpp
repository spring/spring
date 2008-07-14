#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "Map/Ground.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"
#include "Sim/Units/UnitDef.h"
#include "Sim/Units/Unit.h"
#include "Sound.h"
#include "TorpedoLauncher.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CTorpedoLauncher, CWeapon, (NULL));

CR_REG_METADATA(CTorpedoLauncher,(
		CR_MEMBER(tracking),
		CR_RESERVED(8)
		));

CTorpedoLauncher::CTorpedoLauncher(CUnit* owner)
: CWeapon(owner),
	tracking(0)
{
	if (owner) owner->hasUWWeapons=true;
}

CTorpedoLauncher::~CTorpedoLauncher(void)
{
}


void CTorpedoLauncher::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
//		if(!onlyForward){
			wantedDir=targetPos-weaponPos;
			float dist=wantedDir.Length();
			predict=dist/projectileSpeed;
			wantedDir/=dist;
//		}
	}
	CWeapon::Update();
}

void CTorpedoLauncher::Fire(void)
{
	float3 dir;
//	if(onlyForward){
//		dir=owner->frontdir;
//	} else {
		dir=targetPos-weaponMuzzlePos;
		dir.Normalize();
		if(weaponDef->trajectoryHeight>0){
			dir.y+=weaponDef->trajectoryHeight;
			dir.Normalize();
		}
//	}
	float3 startSpeed;
	if (!weaponDef->fixedLauncher) {
		startSpeed = dir * weaponDef->startvelocity;
	}
	else {
		startSpeed = weaponDir * weaponDef->startvelocity;
	}

	SAFE_NEW CTorpedoProjectile(weaponMuzzlePos, startSpeed, owner, areaOfEffect, projectileSpeed,
		tracking, weaponDef->flighttime == 0? (int) (range / projectileSpeed + 25): weaponDef->flighttime,
		targetUnit, weaponDef);

	if (fireSoundId && (!weaponDef->soundTrigger || salvoLeft == salvoSize - 1))
		sound->PlaySample(fireSoundId, owner, fireSoundVolume);
}

bool CTorpedoLauncher::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		if (!(weaponDef->submissile) && unit->unitDef->canhover)
			return false;
		if (!(weaponDef->submissile) && unit->unitDef->canfly && unit->pos.y > 0)
			return false;
	}
	if (!(weaponDef->submissile) && ground->GetHeight2(pos.x, pos.z) > 0)
		return 0;

	float3 dir = pos-weaponMuzzlePos;
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;
	// +0.05f since torpedoes have an unfortunate tendency to hit own ships due to movement
	float spread = (accuracy + sprayAngle) + 0.05f;

	if (avoidFriendly && helper->TestAllyCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, owner)) {
		return false;
	}
	if (avoidNeutral && helper->TestNeutralCone(weaponMuzzlePos, dir, length, spread, owner)) {
		return false;
	}

	return true;
}
