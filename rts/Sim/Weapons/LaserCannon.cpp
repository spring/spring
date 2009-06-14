#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "LaserCannon.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CLaserCannon, CWeapon, (NULL));

CR_REG_METADATA(CLaserCannon,(
	CR_MEMBER(color),
	CR_RESERVED(8)
	));

CLaserCannon::CLaserCannon(CUnit* owner)
: CWeapon(owner)
{
}

CLaserCannon::~CLaserCannon(void)
{
}

void CLaserCannon::Update(void)
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
		if(!onlyForward){
			wantedDir=targetPos-weaponPos;
			wantedDir.Normalize();
		}
		predict=(targetPos-weaponPos).Length()/projectileSpeed;
	}
	CWeapon::Update();
}

bool CLaserCannon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		if (unit->isUnderWater && !weaponDef->waterweapon)
			return false;
	} else {
		if (pos.y < 0 && !weaponDef->waterweapon)
			return false;
	}

	float3 dir = pos - weaponMuzzlePos;
	const float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	if (!onlyForward) {
		// skip ground col testing for aircraft
		float g = ground->LineGroundCol(weaponMuzzlePos, pos);
		if (g > 0 && g < length * 0.9f)
			return false;
	}

	const float spread = (accuracy + sprayAngle) * (1 - owner->limExperience * 0.7f);

	if (avoidFeature && helper->LineFeatureCol(weaponMuzzlePos, dir, length)) {
		return false;
	}
	if (avoidFriendly && helper->TestAllyCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, owner)) {
		return false;
	}
	if (avoidNeutral && helper->TestNeutralCone(weaponMuzzlePos, dir, length, spread, owner)) {
		return false;
	}

	return true;
}

void CLaserCannon::Init(void)
{
	CWeapon::Init();
}

void CLaserCannon::FireImpl()
{
	float3 dir;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)){		//the taairmovetype cant align itself properly, change back when that is fixed
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponMuzzlePos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayAngle+salvoError)*(1-owner->limExperience*0.7f);
	dir.Normalize();

	int fpsSub=0;
	if(owner->directControl)
		fpsSub=6;

	new CLaserProjectile(weaponMuzzlePos, dir * projectileSpeed, owner,
		weaponDef->duration * weaponDef->maxvelocity,
		weaponDef->visuals.color, weaponDef->visuals.color2,
		weaponDef->intensity, weaponDef,
		(int) ((weaponDef->range - fpsSub * 4) / weaponDef->projectilespeed) - fpsSub);
}




