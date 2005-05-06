// Rifle.cpp: implementation of the CRifle class.
//
//////////////////////////////////////////////////////////////////////

#include "StdAfx.h"
#include "Rifle.h"
#include "TracerProjectile.h"
#include "Unit.h"
#include "GameHelper.h"
#include "SmokeProjectile.h"
#include "SyncTracer.h"
#include "Sound.h"
#include "Ground.h"
#include "InfoConsole.h"
#include "HeatCloudProjectile.h"
#include "myMath.h"
#include "Rifle.h"
//#include "mmgr.h"

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
		wantedDir=targetPos-weaponPos;
		wantedDir.Normalize();
	}
	CWeapon::Update();
}

bool CRifle::TryTarget(const float3 &pos,bool userTarget,CUnit* unit)
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

	float3 dir=pos-weaponPos;
	float length=dir.Length();
	dir/=length;

	float g=ground->LineGroundCol(weaponPos,pos);
	if(g>0 && g<length*0.9)
		return false;

	if(helper->TestCone(weaponPos,dir,length,(accuracy+sprayangle)*(1-owner->limExperience*0.9),owner->allyteam,owner)){
		return false;
	}
	return true;
}

void CRifle::Fire(void)
{
	float3 dir=targetPos-weaponPos;
	dir.Normalize();
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.9);
	dir.Normalize();
#ifdef TRACE_SYNC
	tracefile << "Rifle fire: ";
	tracefile << owner->pos.x << " " << dir.x << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
	CUnit* hit;
	float length=helper->TraceRay(weaponPos,dir,range,damages[0],owner,hit);
	if(hit){
		hit->DoDamage(damages,owner,ZeroVector);
		new CHeatCloudProjectile(weaponPos+dir*length,hit->speed*0.9,30,1,owner);
	}
	new CTracerProjectile(weaponPos,dir*projectileSpeed,length,owner);
	new CSmokeProjectile(weaponPos,float3(0,0.0f,0),70,0.1,0.02,owner,0.6f);
	if(fireSoundId)
		sound->PlaySound(fireSoundId,owner,fireSoundVolume);
}
