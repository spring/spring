/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "Game/GameHelper.h"
#include "LogOutput.h"
#include "Map/Ground.h"
#include "myMath.h"
#include "Rifle.h"
#include "Sim/Projectiles/Unsynced/HeatCloudProjectile.h"
#include "Sim/Projectiles/Unsynced/SmokeProjectile.h"
#include "Sim/Projectiles/Unsynced/TracerProjectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "Sync/SyncTracer.h"
#include "mmgr.h"

CR_BIND_DERIVED(CRifle, CWeapon, (NULL));

CR_REG_METADATA(CRifle,(
	CR_RESERVED(8)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRifle::CRifle(CUnit* owner)
: CWeapon(owner)
{
}

CRifle::~CRifle()
{

}

void CRifle::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		weaponMuzzlePos=owner->pos+owner->frontdir*relWeaponMuzzlePos.z+owner->updir*relWeaponMuzzlePos.y+owner->rightdir*relWeaponMuzzlePos.x;
		wantedDir=targetPos-weaponPos;
		wantedDir.Normalize();
	}
	CWeapon::Update();
}

bool CRifle::TryTarget(const float3 &pos, bool userTarget, CUnit* unit)
{
	if (!CWeapon::TryTarget(pos, userTarget, unit))
		return false;

	if (unit) {
		if (unit->isUnderWater)
			return false;
	} else {
		if (pos.y < 0)
			return false;
	}

	float3 dir = pos - weaponMuzzlePos;
	float length = dir.Length();
	if (length == 0)
		return true;

	dir /= length;

	const float g = ground->LineGroundCol(weaponMuzzlePos, pos);
	if (g > 0 && g < length * 0.9f)
		return false;

	const float spread =
		(accuracy + sprayAngle) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight);

	if (helper->TestAllyCone(weaponMuzzlePos, dir, length, spread, owner->allyteam, owner)) {
		// note: check avoidFriendly?
		return false;
	}
	if (avoidNeutral && helper->TestNeutralCone(weaponMuzzlePos, dir, length, spread, owner)) {
		return false;
	}

	return true;
}

void CRifle::FireImpl()
{
	float3 dir = (targetPos - weaponMuzzlePos).Normalize();
	dir +=
		((gs->randVector() * sprayAngle + salvoError) *
		(1.0f - owner->limExperience * weaponDef->ownerExpAccWeight));
	dir.Normalize();

	const CUnit* hit;
	float length=helper->TraceRay(weaponMuzzlePos, dir, range, weaponDef->damages[0], owner, hit, collisionFlags);
	if (hit) {
		CUnit* hitM = uh->units[hit->id];
		hitM->DoDamage(weaponDef->damages, owner, ZeroVector, weaponDef->id);
		new CHeatCloudProjectile(weaponMuzzlePos + dir * length, hit->speed*0.9f, 30, 1, owner);
	}
	new CTracerProjectile(weaponMuzzlePos,dir*projectileSpeed,length,owner);
	new CSmokeProjectile(weaponMuzzlePos,float3(0,0.0f,0),70,0.1f,0.02f,owner,0.6f);
}
