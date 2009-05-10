#include "StdAfx.h"
#include "creg/STL_List.h"
#include "creg/STL_Set.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/TeamHandler.h"
#include "LogOutput.h"
#include "PlasmaRepulser.h"
#include "Rendering/UnitModels/3DOParser.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/Unsynced/RepulseGfx.h"
#include "Sim/Projectiles/Unsynced/ShieldPartProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "WeaponDefHandler.h"
#include "Weapon.h"
#include "mmgr.h"
#include "myMath.h"

CR_BIND_DERIVED(CPlasmaRepulser, CWeapon, (NULL));

CR_REG_METADATA(CPlasmaRepulser, (
	CR_MEMBER(radius),
	CR_MEMBER(sqRadius),
	CR_MEMBER(curPower),
	CR_MEMBER(hitFrames),
	CR_MEMBER(rechargeDelay),
	CR_MEMBER(isEnabled),
	CR_MEMBER(wasDrawn),
	CR_MEMBER(incoming),
	CR_MEMBER(hasGfx),
	CR_MEMBER(visibleShieldParts),
	CR_RESERVED(8)
	));


CPlasmaRepulser::CPlasmaRepulser(CUnit* owner)
: CWeapon(owner),
	curPower(0),
	radius(0),
	sqRadius(0),
	hitFrames(0),
	rechargeDelay(0),
	isEnabled(true),
	wasDrawn(true),
	startShowingShield(true)
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
	else
		curPower=weaponDef->shieldStartingPower;

	CWeapon::Init();
}


void CPlasmaRepulser::Update(void)
{
	const int defHitFrames = weaponDef->visibleShieldHitFrames;
	const bool couldBeVisible = (weaponDef->visibleShield || (defHitFrames > 0));
	const int defRechargeDelay = weaponDef->shieldRechargeDelay;

	rechargeDelay -= (rechargeDelay > 0) ? 1 : 0;

	if (startShowingShield) {
		// one-time iteration when shield first goes online
		// (adds the projectile parts, this assumes owner is
		// not mobile)
		startShowingShield = false;
		if (couldBeVisible) {
			// 32 parts
			for (int y = 0; y < 16; y += 4) {
				for (int x = 0; x < 32; x += 4) {
					visibleShieldParts.push_back(
						new CShieldPartProjectile(owner->pos, x, y, radius,
						                               weaponDef->shieldBadColor,
						                               weaponDef->shieldAlpha,
						                               weaponDef->visuals.texture1, owner)
					);
				}
			}
		}
	}

	if (isEnabled && (curPower < weaponDef->shieldPower) && rechargeDelay <= 0) {
		if (owner->UseEnergy(weaponDef->shieldPowerRegenEnergy * (1.0f / 30.0f))) {
			curPower += weaponDef->shieldPowerRegen * (1.0f / 30.0f);
		}
	}
	weaponPos = owner->pos + (owner->frontdir * relWeaponPos.z)
	                       + (owner->updir    * relWeaponPos.y)
	                       + (owner->rightdir * relWeaponPos.x);

	if (couldBeVisible) {
		float drawAlpha = 0.0f;
		if (hitFrames > 0) {
			drawAlpha += float(hitFrames) / float(defHitFrames);
			hitFrames--;
		}
		if (weaponDef->visibleShield) {
			drawAlpha += 1.0f;
		}
		drawAlpha = std::min(1.0f, drawAlpha * weaponDef->shieldAlpha);
		const bool drawMe = (drawAlpha > 0.0f);

		if (drawMe || wasDrawn) {
			const float colorMix = std::min(1.0f, curPower / std::max(1.0f, weaponDef->shieldPower));
			const float3 color = (weaponDef->shieldGoodColor * colorMix) +
													 (weaponDef->shieldBadColor * (1.0f - colorMix));
			std::list<CShieldPartProjectile*>::iterator si;
			for (si = visibleShieldParts.begin(); si != visibleShieldParts.end(); ++si) {
				CShieldPartProjectile* part = *si;
				part->centerPos = weaponPos;
				part->color = color;
				if (isEnabled) {
					part->baseAlpha = drawAlpha;
				} else {
					part->baseAlpha = 0.0f;
				}
			}
		}
		wasDrawn = drawMe;
	}

	if (isEnabled) {
		for (std::list<CWeaponProjectile*>::iterator pi=incoming.begin();pi!=incoming.end();++pi) {
			const float3 dif = (*pi)->pos-owner->pos;
			if ((*pi)->checkCol && dif.SqLength()<sqRadius && curPower > (*pi)->weaponDef->damages[0]) {
				if (teamHandler->Team(owner->team)->energy > weaponDef->shieldEnergyUse) {
					rechargeDelay = defRechargeDelay;
					if (weaponDef->shieldRepulser) {
						// bounce the projectile
						const int type = (*pi)->ShieldRepulse(this, weaponPos,
						                                      weaponDef->shieldForce,
						                                      weaponDef->shieldMaxSpeed);
						if (type == 0) {
							continue;
						}
						else if (type == 1) {
							owner->UseEnergy(weaponDef->shieldEnergyUse);
							if (weaponDef->shieldPower != 0) {
								curPower -= (*pi)->weaponDef->damages[0];
							}
						}
						else {
							owner->UseEnergy(weaponDef->shieldEnergyUse / 30.0f);
							if (weaponDef->shieldPower != 0) {
								curPower -= (*pi)->weaponDef->damages[0] / 30.0f;
							}
						}

						if (weaponDef->visibleShieldRepulse) {
							std::list<CWeaponProjectile*>::iterator i;
							for (i=hasGfx.begin();i!=hasGfx.end();i++)
								if (*i==*pi) {
									break;
								}
							if (i == hasGfx.end()) {
								hasGfx.insert(hasGfx.end(),*pi);
								const float colorMix = std::min(1.0f, curPower / std::max(1.0f, weaponDef->shieldPower));
								const float3 color = (weaponDef->shieldGoodColor * colorMix) +
								                     (weaponDef->shieldBadColor * (1.0f - colorMix));
								new CRepulseGfx(owner, *pi, radius, color);
							}
						}

						if (defHitFrames > 0) {
							hitFrames = defHitFrames;
						}
					}
					else {
					  // kill the projectile
						if (owner->UseEnergy(weaponDef->shieldEnergyUse)) {
							if (weaponDef->shieldPower != 0) {
								curPower -= (*pi)->weaponDef->damages[0];
							}
							(*pi)->Collision(owner);
							if (defHitFrames > 0) {
								hitFrames = defHitFrames;
							}
						}
					}
				} else {
					// Calculate the amount of energy we wanted to pull
					/*
					Domipheus: TODO Commented out for now, ShieldRepulse has side effects, design needs altering.

					if(weaponDef->shieldRepulser) {	//bounce the projectile
						int type=(*pi)->ShieldRepulse(this,weaponPos,weaponDef->shieldForce,weaponDef->shieldMaxSpeed);
						if (type==1){
							teamHandler->Team(owner->team)->energyPullAmount += weaponDef->shieldEnergyUse;
						} else {
							teamHandler->Team(owner->team)->energyPullAmount += weaponDef->shieldEnergyUse/30.0f;
						}
					} else {						//kill the projectile
						teamHandler->Team(owner->team)->energyPullAmount += weaponDef->shieldEnergyUse;
					}*/
				}
			}
		}
	}
}


void CPlasmaRepulser::SlowUpdate(void)
{
	const int piece = owner->script->QueryWeapon(weaponNum);
	relWeaponPos = owner->script->GetPiecePos(piece);
	weaponPos = owner->pos + (owner->frontdir * relWeaponPos.z)
	                       + (owner->updir    * relWeaponPos.y)
	                       + (owner->rightdir * relWeaponPos.x);
	owner->script->AimShieldWeapon(this);
}


void CPlasmaRepulser::NewProjectile(CWeaponProjectile* p)
{
	if (weaponDef->smartShield && teamHandler->AlliedTeams(p->owner()->team, owner->team)) {
		return;
	}

	float3 dir;
	if (p->targetPos!=ZeroVector) {
		dir = p->targetPos-p->pos; // assume that it will travel roughly in the direction of the targetpos if it have one
	} else {
		dir = p->speed;            // otherwise assume speed will hold constant
	}
	dir.y = 0;
	dir.Normalize();
	float3 dif = owner->pos-p->pos;

	if (weaponDef->exteriorShield && (dif.SqLength() < sqRadius)) {
		return;
	}

	float closeLength=dif.dot(dir);
	if (closeLength < 0) {
		closeLength = 0;
	}
	float3 closeVect=dif-dir*closeLength;

	if (closeVect.SqLength2D() < Square(radius * 1.5f + 400)) {
		incoming.push_back(p);
		AddDeathDependence(p);
	}
}


float CPlasmaRepulser::NewBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir)
{
	if (!isEnabled) {
		return -1;
	}
	if (emitter->weaponDef->damages[0] > curPower) {
		return -1;
	}
	if (weaponDef->smartShield && teamHandler->AlliedTeams(emitter->owner->team,owner->team)) {
		return -1;
	}

	const float3 dif = weaponPos - start;

	if (weaponDef->exteriorShield && dif.SqLength() < sqRadius) {
		return -1;
	}

	const float closeLength = dif.dot(dir);

	if (closeLength < 0) {
		return -1;
	}

	const float3 closeVect = dif-dir*closeLength;

	const float tmp = sqRadius - closeVect.SqLength();
	if ((tmp > 0) && (length > (closeLength - sqrt(tmp)))) {
		float colLength = closeLength - sqrt(tmp);
		float3 colPoint = start + dir * colLength;
		float3 normal = colPoint - weaponPos;
		normal.Normalize();
		newDir = dir-normal*normal.dot(dir) * 2;
		return colLength;
	}
	return -1;
}


void CPlasmaRepulser::DependentDied(CObject* o)
{
	incoming.remove((CWeaponProjectile*)o);
	ListErase<CWeaponProjectile*>(hasGfx, (CWeaponProjectile*)o);
	CWeapon::DependentDied(o);
}


bool CPlasmaRepulser::BeamIntercepted(CWeapon* emitter, float damageMultiplier)
{
	if (weaponDef->shieldPower > 0) {
		curPower -= emitter->weaponDef->damages[0] * damageMultiplier;
	}
	return weaponDef->shieldRepulser;
}
