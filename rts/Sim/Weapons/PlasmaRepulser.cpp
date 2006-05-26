#include "StdAfx.h"
#include "PlasmaRepulser.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "Game/UI/InfoConsole.h"
#include "Sim/Projectiles/RepulseGfx.h"
#include "Sim/Projectiles/WeaponProjectile.h"
#include "Sim/Units/COB/CobInstance.h"
#include "Sim/Units/COB/CobFile.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Projectiles/ShieldPartProjectile.h"
#include "Game/Team.h"
#include "mmgr.h"

CPlasmaRepulser::CPlasmaRepulser(CUnit* owner)
: CWeapon(owner),
	radius(0),
	sqRadius(0),
	curPower(0)
{
	interceptHandler.AddPlasmaRepulser(this);
}

CPlasmaRepulser::~CPlasmaRepulser(void)
{
	interceptHandler.RemovePlasmaRepulser(this);

	for(std::list<CShieldPartProjectile*>::iterator si=visibleShieldParts.begin();si!=visibleShieldParts.end();++si){
		(*si)->deleteMe=true;
	}
}

void CPlasmaRepulser::Init(void)
{
	radius=weaponDef->shieldRadius;
	sqRadius=radius*radius;

	if(weaponDef->shieldPower==0)
		curPower=99999999999.0f;

	if(weaponDef->visibleShield){
		for(int y=0;y<16;y+=4){
			for(int x=0;x<32;x+=4){
				visibleShieldParts.push_back(new CShieldPartProjectile(owner->pos,x,y,radius,weaponDef->shieldBadColor,weaponDef->shieldAlpha,owner));
			}
		}
	}
	CWeapon::Init();
}
void CPlasmaRepulser::Update(void)
{
	if(curPower<weaponDef->shieldPower){
		if(owner->UseEnergy(weaponDef->shieldPowerRegenEnergy*(1.0/30)))
			curPower+=weaponDef->shieldPowerRegen*(1.0/30);
	}
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;

	if(weaponDef->visibleShield){
		float colorMix=min(1.f,curPower/(max(1.f,weaponDef->shieldPower)));
		float3 color=weaponDef->shieldGoodColor*colorMix+weaponDef->shieldBadColor*(1-colorMix);
		for(std::list<CShieldPartProjectile*>::iterator si=visibleShieldParts.begin();si!=visibleShieldParts.end();++si){
			(*si)->centerPos=weaponPos;
			(*si)->color=color;
		}
	}

	for(std::list<CWeaponProjectile*>::iterator pi=incoming.begin();pi!=incoming.end();++pi){
		float3 dif=(*pi)->pos-owner->pos;
		if((*pi)->checkCol && dif.SqLength()<sqRadius && curPower > (*pi)->damages[0] && gs->Team(owner->team)->energy > weaponDef->shieldEnergyUse){
			if(weaponDef->shieldRepulser){	//bounce the projectile
				int type=(*pi)->ShieldRepulse(this,weaponPos,weaponDef->shieldForce,weaponDef->shieldMaxSpeed);
				if(type==0){
					continue;
				} else if (type==1){
					owner->UseEnergy(weaponDef->shieldEnergyUse);
					curPower-=(*pi)->damages[0];
				} else {
					owner->UseEnergy(weaponDef->shieldEnergyUse/30.0f);
					curPower-=(*pi)->damages[0]/30.0f;
				}
				if(weaponDef->visibleShieldRepulse){
					if(hasGfx.find(*pi)==hasGfx.end()){
						hasGfx.insert(*pi);
						float colorMix=min(1.f,curPower/(max(1.f,weaponDef->shieldPower)));
						float3 color=weaponDef->shieldGoodColor*colorMix+weaponDef->shieldBadColor*(1-colorMix);
						new CRepulseGfx(owner,*pi,radius,color);
					}
				}
			} else {						//kill the projectile
				if(owner->UseEnergy(weaponDef->shieldEnergyUse)){
					curPower-=(*pi)->damages[0];
					(*pi)->Collision();
				}
			}
		}
	}
}

void CPlasmaRepulser::SlowUpdate(void)
{
	std::vector<long> args;
	args.push_back(0);
	owner->cob->Call(COBFN_QueryPrimary+weaponNum,args);
	relWeaponPos=owner->localmodel->GetPiecePos(args[0]);
	weaponPos=owner->pos+owner->frontdir*relWeaponPos.z+owner->updir*relWeaponPos.y+owner->rightdir*relWeaponPos.x;
}

void CPlasmaRepulser::NewProjectile(CWeaponProjectile* p)
{
	if(weaponDef->smartShield && gs->AlliedTeams(p->owner->team,owner->team))
		return;

	float3 dir;
	if(p->targetPos!=ZeroVector)
		dir=p->targetPos-p->pos;	//assume that it will travel roughly in the direction of the targetpos if it have one
	else
		dir=p->speed;				//otherwise assume speed will hold constant
	dir.y=0;
	dir.Normalize();
	float3 dif=owner->pos-p->pos;

	if(weaponDef->exteriorShield && dif.SqLength() < sqRadius)
		return;

	float closeLength=dif.dot(dir);
	if(closeLength<0)
		closeLength=0;
	float3 closeVect=dif-dir*closeLength;

	if(closeVect.Length2D()<radius*1.5+400){
		incoming.push_back(p);
		AddDeathDependence(p);
	}
}

float CPlasmaRepulser::NewBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir)
{
	if(emitter->damages[0] > curPower)
		return -1;

	if(weaponDef->smartShield && gs->AlliedTeams(emitter->owner->team,owner->team))
		return -1;

	float3 dif=weaponPos-start;

	if(weaponDef->exteriorShield && dif.SqLength() < sqRadius)
		return -1;

	float closeLength=dif.dot(dir);

	if(closeLength<0)
		return -1;

	float3 closeVect=dif-dir*closeLength;

	float tmp = sqRadius - closeVect.SqLength();
	if(tmp > 0 && length > closeLength-sqrt(tmp)){
		float colLength=closeLength-sqrt(tmp);
		float3 colPoint=start+dir*colLength;
		float3 normal=colPoint-weaponPos;
		normal.Normalize();
		newDir=dir-normal*normal.dot(dir)*2;
		return colLength;
	}
	return -1;
}

void CPlasmaRepulser::DependentDied(CObject* o)
{
	incoming.remove((CWeaponProjectile*)o);
	hasGfx.erase((CWeaponProjectile*)o);
	CWeapon::DependentDied(o);
}

bool CPlasmaRepulser::BeamIntercepted(CWeapon* emitter)
{
	if(weaponDef->shieldPower > 0)
		curPower-=emitter->damages[0];

	return weaponDef->shieldRepulser;
}
