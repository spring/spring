#include "StdAfx.h"
// Cannon.cpp: implementation of the CCannon class.
//
//////////////////////////////////////////////////////////////////////

#include "Cannon.h"
#include "Sim/Units/Unit.h"
#include "Sim/Projectiles/ExplosiveProjectile.h"
#include "Sim/Projectiles/HeatCloudProjectile.h"
#include "LogOutput.h"
#include "Sim/Projectiles/SmokeProjectile.h"
#include "Sync/SyncTracer.h"
#include "Sound.h"
#include "Map/Ground.h"
#include "Game/GameHelper.h"
#include "myMath.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "WeaponDefHandler.h"
#include "Rendering/Env/BaseWater.h"
#include "mmgr.h"

CR_BIND_DERIVED(CCannon, CWeapon, (NULL));

CR_REG_METADATA(CCannon,(
	CR_MEMBER(maxPredict),
	CR_MEMBER(minPredict),
	CR_MEMBER(highTrajectory),
	CR_MEMBER(selfExplode)
	));

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCannon::CCannon(CUnit* owner)
: CWeapon(owner)
{
	highTrajectory=false;
}

void CCannon::Init(void)
{
	if(highTrajectory){
		maxPredict=projectileSpeed*2/-gs->gravity;
		minPredict=projectileSpeed*1.41f/-gs->gravity;
	} else {
		maxPredict=projectileSpeed*1.41f/-gs->gravity;
	}
	CWeapon::Init();
}

CCannon::~CCannon()
{

}

void CCannon::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos + owner->frontdir*relWeaponPos.z
				 + owner->updir*relWeaponPos.y + owner->rightdir*relWeaponPos.x;
		float3 diff = targetPos-weaponPos;
		wantedDir = GetWantedDir(diff);
		float speed2D = wantedDir.Length2D() * projectileSpeed;
		predict = ((speed2D == 0) ? 1 : (diff.Length2D()/speed2D));
	} else {
		predict=0;
	}
	CWeapon::Update();
}

bool CCannon::TryTarget(const float3 &pos,bool userTarget,CUnit* unit)
{

	if(!CWeapon::TryTarget(pos,userTarget,unit))
	{
		return false;
	}

	if(unit)
	{
		if(unit->isUnderWater)
		{
			return false;
		}
	} else {
		if(pos.y<0)
		{
			return false;
		}
	}

	if (projectileSpeed == 0)
	{
		return true;
	}
	float3 dif(pos-weaponPos);

	float3 dir(GetWantedDir(dif));
	
	if(dir.SqLength() == 0){
		return false;
	}

	float3 flatdir(dif.x,0,dif.z);
	float flatlength=flatdir.Length();
	if(flatlength==0) {
		return true;
	}
	flatdir/=flatlength;

	float gc=ground->TrajectoryGroundCol(weaponPos, flatdir, flatlength-10,
			dir.y , gs->gravity / (projectileSpeed * projectileSpeed) * 0.5f);
	if(gc>0) {
		return false;
	}

/*	gc=ground->LineGroundCol(wpos+dir*(length*0.5f),pos,false);
	if(gc>0 && gc<length*0.40f)
		return false;
*/
	if(avoidFriendly && helper->TestTrajectoryCone(weaponPos, flatdir,
		flatlength-30, dir.y, gs->gravity /
		(projectileSpeed * projectileSpeed) * 0.5f,
		(accuracy+sprayangle) * 0.6f * (1-owner->limExperience * 0.9f) * 0.9f,
		3, owner->allyteam, owner))
	{
		return false;
	}
/*	if(helper->TestCone(weaponPos,dir,length*0.5f,(accuracy+sprayangle)*1.2f*(1-owner->limExperience*0.9f)*0.9f,owner->allyteam,owner)){
		return false;
	}
	float3 dir2(dif);
	dir2.y+=predictTime*predictTime*gs->gravity;		//compensate for the earlier up prediction
	dir2.Normalize();
	if(helper->TestCone(weaponPos+dir*(length*0.5f),dir2,length*0.5f,(accuracy+sprayangle)*!userTarget*(1-owner->limExperience*0.9f)*0.6f,owner->allyteam,owner)){
		return false;
	}*/
	return true;
}


void CCannon::Fire(void)
{
	float3 diff = targetPos-weaponPos;
	float3 dir=GetWantedDir(diff);
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.9f);
	dir.Normalize();
#ifdef TRACE_SYNC
	tracefile << "Cannon fire: ";
	tracefile << owner->pos.x << " " << dir.x << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
	int ttl = 0;
	float sqSpeed2D = dir.SqLength2D() * projectileSpeed * projectileSpeed;
	int predict = ceil((sqSpeed2D == 0) ? (-2 * projectileSpeed * dir.y / gs->gravity)
			: sqrt(diff.SqLength2D() / sqSpeed2D));
	if(selfExplode) {
		ttl=(int)(predict+gs->randFloat()*2.5f-0.5f);
	} else {
		ttl=predict*2;
	}
	SAFE_NEW CExplosiveProjectile(weaponPos,dir*projectileSpeed,owner,weaponDef, ttl,areaOfEffect);
	//CWeaponProjectile::CreateWeaponProjectile(weaponPos,owner->speed,owner, NULL, float3(0,0,0), weaponDef);

//	SAFE_NEW CSmokeProjectile(weaponPos,dir*0.01f,90,0.1f,0.02f,owner,0.6f);
//	CHeatCloudProjectile* p=SAFE_NEW CHeatCloudProjectile(weaponPos,dir*0.02f,8,0.6f,owner);
//	p->Update();
//	p->maxheat=p->heat;
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);
	if(weaponPos.y<30)
 		water->AddExplosion(weaponPos,damages[0]*0.1f,sqrt(damages[0])+80);
}

void CCannon::SlowUpdate(void)
{
	if(owner->useHighTrajectory!=highTrajectory){
		highTrajectory=owner->useHighTrajectory;
		if(highTrajectory){
			maxPredict=projectileSpeed*2/-gs->gravity;
			minPredict=projectileSpeed*1.41f/-gs->gravity;
		} else {
			maxPredict=projectileSpeed*1.41f/-gs->gravity;
		}
	}
	CWeapon::SlowUpdate();
}

bool CCannon::AttackGround(float3 pos,bool userTarget)
{
#ifdef DIRECT_CONTROL_ALLOWED
	if(owner->directControl)		//mostly prevents firing longer than max range using fps mode
		pos.y=ground->GetHeight(pos.x,pos.z);
#endif

	return CWeapon::AttackGround(pos,userTarget);
}

float3 CCannon::GetWantedDir(const float3& diff)
{
	float Dsq = diff.SqLength();
	float DFsq = diff.SqLength2D();
	float g = gs->gravity;
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
	return dir;
}

float CCannon::GetRange2D(float yDiff) const
{
	float root1 = 1 + 2*gs->gravity*yDiff/(projectileSpeed*projectileSpeed);
	if(root1 < 0){
		return 0;
	} else {
		return range * sqrt(root1);
	}
}
