#include "StdAfx.h"
#include "EmgCannon.h"
#include "Game/GameHelper.h"
#include "Sim/Misc/Team.h"
#include "Map/Ground.h"
#include "Sim/MoveTypes/AirMoveType.h"
#include "Sim/Projectiles/WeaponProjectiles/EmgProjectile.h"
#include "Sim/Units/Unit.h"
#include "Sync/SyncTracer.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"

CR_BIND_DERIVED(CEmgCannon, CWeapon, (NULL));

CR_REG_METADATA(CEmgCannon,(
				CR_RESERVED(8)
				));

CEmgCannon::CEmgCannon(CUnit* owner)
: CWeapon(owner)
{
}

CEmgCannon::~CEmgCannon(void)
{
}

void CEmgCannon::Update(void)
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

bool CEmgCannon::TryTarget(const float3& pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (!weaponDef->waterweapon) {
		if (unit) {
			if (unit->isUnderWater)
				return false;
		} else {
			if (pos.y < 0)
				return false;
		}
	}

	float3 dir = pos - weaponMuzzlePos;
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	float g = ground->LineGroundCol(weaponMuzzlePos, pos);
	if (g > 0 && g < length * 0.9f)
		return false;

	float spread = (accuracy + sprayAngle) * (1 - owner->limExperience * 0.5f);

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

void CEmgCannon::Init(void)
{
	CWeapon::Init();
}

void CEmgCannon::FireImpl()
{
	float3 dir;
	if(onlyForward && dynamic_cast<CAirMoveType*>(owner->moveType)){		//the taairmovetype cant align itself properly, change back when that is fixed
		dir=owner->frontdir;
	} else {
		dir=targetPos-weaponMuzzlePos;
		dir.Normalize();
	}
	dir+=(gs->randVector()*sprayAngle+salvoError)*(1-owner->limExperience*0.5f);
	dir.Normalize();

	new CEmgProjectile(weaponMuzzlePos, dir * projectileSpeed, owner,
		weaponDef->visuals.color, weaponDef->intensity, (int) (range / projectileSpeed),
		weaponDef);
}




