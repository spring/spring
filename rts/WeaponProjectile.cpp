#include "StdAfx.h"
#include "WeaponProjectile.h"
#include "WeaponDefHandler.h"
#include "Sound.h"
#include "3DOParser.h"
#include "myGL.h"
#include "TextureHandler.h"
#include "Ground.h"
#include "GameHelper.h"
#include "ModelProjectile.h"
#include "LaserProjectile.h"
#include "FireBallProjectile.h"
#include "Matrix44f.h"
#include "Feature.h"
#include "Unit.h"
//#include "mmgr.h"

CWeaponProjectile::CWeaponProjectile(const float3& pos,const float3& speed,CUnit* owner, CUnit* target,const float3 &targetPos, WeaponDef *weaponDef,CWeaponProjectile* interceptTarget) : 
	CProjectile(pos,speed,owner),
	weaponDef(weaponDef),
	target(target),
	targetPos(targetPos),
	startpos(pos),
	targeted(false),
	interceptTarget(interceptTarget)
{
	if(interceptTarget){
		interceptTarget->targeted=true;
		AddDeathDependence(interceptTarget);
	}
}

CWeaponProjectile::~CWeaponProjectile(void)
{
}

CWeaponProjectile *CWeaponProjectile::CreateWeaponProjectile(const float3& pos,const float3& speed, CUnit* owner, CUnit *target, const float3 &targetPos, WeaponDef *weaponDef)
{
	switch(weaponDef->visuals.renderType)
	{
	case WEAPON_RENDERTYPE_MODEL:
		return new CModelProjectile(pos,speed,owner,target,targetPos,weaponDef);
		break;
	case WEAPON_RENDERTYPE_LASER:
		return new CLaserProjectile(pos, speed, owner, weaponDef->damages, 30, weaponDef->visuals.color, 0.8, weaponDef, weaponDef->range/weaponDef->movement.projectilespeed);
		break;
	case WEAPON_RENDERTYPE_PLASMA:
		break;
	case WEAPON_RENDERTYPE_FIREBALL:
		return new CFireBallProjectile(pos,speed,owner,target,targetPos,weaponDef);
		break;
	}

	//gniarf!
	return NULL;
}

void CWeaponProjectile::Collision()
{
	if(!weaponDef->movement.noExplode || gs->frameNum&1)
		helper->Explosion(pos,weaponDef->damages,weaponDef->areaOfEffect,owner,true,weaponDef->movement.noExplode? 0.3:1,weaponDef->movement.noExplode);

	if(weaponDef->soundhit.id)
		sound->PlaySound(weaponDef->soundhit.id,this,weaponDef->soundhit.volume);

	if(!weaponDef->movement.noExplode)
		CProjectile::Collision();
	else
	{
		if(TraveledRange())
			CProjectile::Collision();
	}

}

void CWeaponProjectile::Collision(CFeature* feature)
{
	if(gs->randFloat()>weaponDef->fireStarter)
		feature->StartFire();

	Collision();
}

void CWeaponProjectile::Collision(CUnit* unit)
{
	if(!weaponDef->movement.noExplode || gs->frameNum&1)
		helper->Explosion(pos,weaponDef->damages,weaponDef->areaOfEffect,owner,true,weaponDef->movement.noExplode? 0.3:1,weaponDef->movement.noExplode);

	if(weaponDef->soundhit.id)
		sound->PlaySound(weaponDef->soundhit.id,this,weaponDef->soundhit.volume);

	if(!weaponDef->movement.noExplode)
		CProjectile::Collision(unit);
	else
	{
		if(TraveledRange())
			CProjectile::Collision();
	}
}

void CWeaponProjectile::Update()
{
	//pos+=speed;

	//if(weaponDef->movement.gravityAffected)
	//	speed.y+=gs->gravity;


	//if(weaponDef->movement.noExplode)
	//{
 //       if(TraveledRange())
	//		CProjectile::Collision();
	//}

	//if(speed.Length()<weaponDef->movement.maxvelocity)
	//	speed += dir*weaponDef->movement.weaponacceleration

	CProjectile::Update();
}

bool CWeaponProjectile::TraveledRange()
{
	float trange = (pos-startpos).Length();
	if(trange>weaponDef->range)
		return true;
	return false;
}

void CWeaponProjectile::DrawUnitPart()
{

}

void CWeaponProjectile::DependentDied(CObject* o)
{
	if(o==interceptTarget)
		interceptTarget=0;

	if(o==target)
		target=0;

	CProjectile::DependentDied(o);
}
