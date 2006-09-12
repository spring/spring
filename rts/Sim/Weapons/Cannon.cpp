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
#include "SyncTracer.h"
#include "Sound.h"
#include "Map/Ground.h"
#include "Game/GameHelper.h"
#include "myMath.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "WeaponDefHandler.h"
#include "mmgr.h"
#include "Rendering/Env/BaseWater.h"

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
		minPredict=projectileSpeed*1.41/-gs->gravity;
	} else {
		maxPredict=projectileSpeed*1.41/-gs->gravity;
	}
	CWeapon::Init();
}

CCannon::~CCannon()
{

}

void CCannon::Update()
{
	if(targetType!=Target_None){
		weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
		if(highTrajectory){
			if(predict>maxPredict)
				predict=maxPredict;
			if(predict<minPredict)
				predict=minPredict;
			
			float3 dif=targetPos-weaponPos;
			float k1=dif.SqLength2D();
			float h=projectileSpeed*predict;
			h=h*h;

			float k2=h-k1;
			if(k2>0){
				k2=sqrt(k2);
				k2+=weaponPos.y-targetPos.y;
				k2/=-gs->gravity*0.5;
				if(k2>0)							//why is this needed really (i no longer understand my equations :) )
					predict=sqrt(k2);
			}
			wantedDir=targetPos-float3(0,5,0)-weaponPos;
			wantedDir.y-=predict*predict*gs->gravity*0.5;
			wantedDir.Normalize();
		} else {
			float3 dif=targetPos-weaponPos;
			dif.y-=predict*predict*gs->gravity*0.5;
			predict=dif.Length()/projectileSpeed;

			if(predict>maxPredict)
				predict=maxPredict;
			wantedDir=dif;
			wantedDir.Normalize();
		}
	} else {
		predict=0;
	}
	CWeapon::Update();
}

bool CCannon::TryTarget(const float3 &pos,bool userTarget,CUnit* unit)
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

	float3 dif(pos-weaponPos);
	float predictTime=dif.Length()/projectileSpeed;
	if(predictTime==0)
		return true;
	if(highTrajectory)
		predictTime=(minPredict+maxPredict)*0.5;
	dif.y-=predictTime*predictTime*gs->gravity*0.5;
	float length=dif.Length();

	float3 dir(dif/length);

	float3 flatdir(dif.x,0,dif.z);
	float flatlength=flatdir.Length();
	if(flatlength==0)
		return true;
	flatdir/=flatlength;

	float gc=ground->TrajectoryGroundCol(weaponPos,flatdir,flatlength-10,dif.y/flatlength,gs->gravity/(projectileSpeed*projectileSpeed)*0.5);
	if(gc>0)
		return false;

/*	gc=ground->LineGroundCol(wpos+dir*(length*0.5),pos,false);
	if(gc>0 && gc<length*0.40)
		return false;
*/
	if(avoidFriendly && helper->TestTrajectoryCone(weaponPos,flatdir,flatlength-30,dif.y/flatlength,gs->gravity/(projectileSpeed*projectileSpeed)*0.5,(accuracy+sprayangle)*0.6*(1-owner->limExperience*0.9)*0.9,3,owner->allyteam,owner)){
		return false;
	}
/*	if(helper->TestCone(weaponPos,dir,length*0.5,(accuracy+sprayangle)*1.2*(1-owner->limExperience*0.9)*0.9,owner->allyteam,owner)){
		return false;
	}
	float3 dir2(dif);
	dir2.y+=predictTime*predictTime*gs->gravity;		//compensate for the earlier up prediction
	dir2.Normalize();
	if(helper->TestCone(weaponPos+dir*(length*0.5),dir2,length*0.5,(accuracy+sprayangle)*!userTarget*(1-owner->limExperience*0.9)*0.6,owner->allyteam,owner)){
		return false;
	}*/
	return true;
}


void CCannon::Fire(void)
{
	float3 dir=targetPos-weaponPos;

	dir.y-=predict*predict*gs->gravity*0.5;
	dir.Normalize();
	dir+=(gs->randVector()*sprayangle+salvoError)*(1-owner->limExperience*0.9);
	dir.Normalize();
#ifdef TRACE_SYNC
	tracefile << "Cannon fire: ";
	tracefile << owner->pos.x << " " << dir.x << " " << targetPos.x << " " << targetPos.y << " " << targetPos.z << "\n";
#endif
	int ttl=10000;
	if(selfExplode)
		ttl=(int)(predict+gs->randFloat()*2.5-0.5);
	new CExplosiveProjectile(weaponPos,dir*projectileSpeed,owner,damages,weaponDef, ttl,areaOfEffect);
	//CWeaponProjectile::CreateWeaponProjectile(weaponPos,owner->speed,owner, NULL, float3(0,0,0), weaponDef);

//	new CSmokeProjectile(weaponPos,dir*0.01,90,0.1,0.02,owner,0.6f);
//	CHeatCloudProjectile* p=new CHeatCloudProjectile(weaponPos,dir*0.02,8,0.6,owner);
//	p->Update();
//	p->maxheat=p->heat;
	if(fireSoundId && (!weaponDef->soundTrigger || salvoLeft==salvoSize-1))
		sound->PlaySample(fireSoundId,owner,fireSoundVolume);
	if(weaponPos.y<30)
 		water->AddExplosion(weaponPos,damages[0]*0.1,sqrt(damages[0])+80);
}

void CCannon::SlowUpdate(void)
{
	if(owner->useHighTrajectory!=highTrajectory){
		highTrajectory=owner->useHighTrajectory;
		if(highTrajectory){
			maxPredict=projectileSpeed*2/-gs->gravity;
			minPredict=projectileSpeed*1.41/-gs->gravity;
		} else {
			maxPredict=projectileSpeed*1.41/-gs->gravity;
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
