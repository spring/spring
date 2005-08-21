#include "StdAfx.h"
#include "PlasmaRepulser.h"
#include "InterceptHandler.h"
#include "Projectile.h"
#include "Unit.h"
#include "WeaponDefHandler.h"
#include "InfoConsole.h"
#include "RepulseGfx.h"
//#include "mmgr.h"

CPlasmaRepulser::CPlasmaRepulser(CUnit* owner)
: CWeapon(owner)
{
	interceptHandler.AddPlasmaRepulser(this);
}

CPlasmaRepulser::~CPlasmaRepulser(void)
{
	interceptHandler.RemovePlasmaRepulser(this);
}

void CPlasmaRepulser::Update(void)
{
	for(std::list<CProjectile*>::iterator pi=incoming.begin();pi!=incoming.end();++pi){
		float3 dif=(*pi)->pos-owner->pos;
		if(dif.SqLength()<weaponDef->repulseRange*weaponDef->repulseRange){
			float3 dir=dif;
			dir.Normalize();
			if(dir.dot((*pi)->speed)<weaponDef->repulseSpeed && owner->UseEnergy(weaponDef->repulseEnergy)){
				(*pi)->speed+=dir*weaponDef->repulseForce;
				if(hasGfx.find(*pi)==hasGfx.end()){
					hasGfx.insert(*pi);
					new CRepulseGfx(owner,*pi,weaponDef->repulseSpeed,weaponDef->repulseRange);
				}
			}
		}
	}
}

void CPlasmaRepulser::SlowUpdate(void)
{
}

void CPlasmaRepulser::NewPlasmaProjectile(CProjectile* p)
{
	float3 dir=p->speed;
	dir.y=0;
	dir.Normalize();
	float3 dif=owner->pos-p->pos;
	float closeLength=dif.dot(dir);
	if(closeLength<0)
		closeLength=0;
	float3 closeVect=dif-dir*closeLength;

	if(closeVect.Length2D()<weaponDef->repulseRange){
		incoming.push_back(p);
		AddDeathDependence(p);
	}
}

void CPlasmaRepulser::DependentDied(CObject* o)
{
	incoming.remove((CProjectile*)o);
	hasGfx.erase((CProjectile*)o);
	CWeapon::DependentDied(o);
}

