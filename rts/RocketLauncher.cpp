// RocketLauncher.cpp: implementation of the CRocketLauncher class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "RocketLauncher.h"
#include "RocketProjectile.h"
#include "Unit.h"
#include "Sound.h"
#include "Ground.h"
#include "GameHelper.h"
#include "SyncTracer.h"
#include "myMath.h"
//#include "mmgr.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRocketLauncher::CRocketLauncher(CUnit* owner)
: CWeapon(owner)
{
}

CRocketLauncher::~CRocketLauncher()
{

}

void CRocketLauncher::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

		wantedDir=targetPos-float3(0,0.3f,0)-weaponPos;
		float dist=wantedDir.Length();
		predict=dist*1.3/projectileSpeed;

		wantedDir/=dist;
		wantedDir.y+=0.7f;
		wantedDir.Normalize();
	} else {
		predict=0;
	}
	CWeapon::Update();
}

bool CRocketLauncher::TryTarget(const float3 &pos,bool userTarget,CUnit* unit)
{
	if(!CWeapon::TryTarget(pos,userTarget,unit))
		return false;

	if(unit){
		if(unit->isUnderWater)
			return false;
	} else {
		if(pos.y<0)
			return false;
	}

	float3 dir(pos-weaponPos);
	dir.y=dir.Length();
	float length=dir.Length();
	dir/=length;

	float gc=ground->LineGroundCol(weaponPos,weaponPos+dir*(length*0.5));
	if(gc>0)
		return false;

	if(helper->TestCone(weaponPos,dir,length*0.5,(accuracy+sprayangle)*(1-owner->limExperience*0.4),owner->allyteam,owner)){
		return false;
	}
#ifdef TRACE_SYNC
	tracefile << "Launcher have target: ";
	tracefile << owner->id << " " << pos.x << " " << pos.y << " " << pos.z << " " << gs->frameNum << " " << length << "\n";
#endif
	return true;
}

void CRocketLauncher::Fire(void)
{
#ifdef TRACE_SYNC
	tracefile << "Launcher firing: ";
	tracefile << owner->id << " " << gs->frameNum << "\n";
#endif
	float3 dir=targetPos-weaponPos;
	float dist=dir.Length();
	//		float vDif=dir.y+hDist*0.7;
	dir/=dist;
	dir.y+=0.7f;
	dir.Normalize();
	float time=dist/projectileSpeed;
	//		float grav=vDif/(time*time)*1.9f;
	new CRocketProjectile(weaponPos,dir*0.01f,owner,damages,gs->gravity*0.8,(accuracy+sprayangle),projectileSpeed,targetPos,0.007*(1+owner->limExperience*2), weaponDef);
	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}
