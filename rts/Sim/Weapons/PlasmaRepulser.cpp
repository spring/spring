/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "creg/STL_List.h"
#include "creg/STL_Set.h"
#include "PlasmaRepulser.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/Projectile.h"
#include "Sim/Projectiles/Unsynced/RepulseGfx.h"
#include "Sim/Projectiles/Unsynced/ShieldPartProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/COB/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDefHandler.h"
#include "Sim/Weapons/Weapon.h"
#include "System/mmgr.h"
#include "System/myMath.h"

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

	if (!isEnabled) {
		return;
	}

	for (std::list<CWeaponProjectile*>::iterator pi = incoming.begin(); pi != incoming.end(); ++pi) {
		CWeaponProjectile* pro = *pi;

		if (!pro->checkCol) {
			continue;
		}

		if ((pro->pos - owner->pos).SqLength() > sqRadius) {
			// projectile does not hit the shield, don't touch it
			continue;
		}

		if (luaRules && luaRules->ShieldPreDamaged(pro, this, owner, weaponDef->shieldRepulser)) {
			// gadget handles the collision event, don't touch the projectile
			continue;
		}

		if (curPower < pro->weaponDef->damages[0]) {
			// shield does not have enough power, don't touch the projectile
			continue;
		}

		if (teamHandler->Team(owner->team)->energy < weaponDef->shieldEnergyUse) {
			// team does not have enough energy, don't touch the projectile
			continue;
		}

		rechargeDelay = defRechargeDelay;

		if (weaponDef->shieldRepulser) {
			// bounce the projectile
			const int type = pro->ShieldRepulse(this, weaponPos,
													weaponDef->shieldForce,
													weaponDef->shieldMaxSpeed);

			if (type == 0) {
				continue;
			} else if (type == 1) {
				owner->UseEnergy(weaponDef->shieldEnergyUse);

				if (weaponDef->shieldPower != 0) {
					curPower -= pro->weaponDef->damages[0];
				}
			} else {
				owner->UseEnergy(weaponDef->shieldEnergyUse / 30.0f);

				if (weaponDef->shieldPower != 0) {
					curPower -= pro->weaponDef->damages[0] / 30.0f;
				}
			}

			if (weaponDef->visibleShieldRepulse) {
				std::list<CWeaponProjectile*>::iterator i;

				for (i = hasGfx.begin(); i != hasGfx.end(); ++i) {
					if (*i == pro) {
						break;
					}
				}

				if (i == hasGfx.end()) {
					hasGfx.insert(hasGfx.end(), pro);

					const float colorMix = std::min(1.0f, curPower / std::max(1.0f, weaponDef->shieldPower));
					const float3 color =
						(weaponDef->shieldGoodColor * colorMix) +
						(weaponDef->shieldBadColor * (1.0f - colorMix));

					new CRepulseGfx(owner, pro, radius, color);
				}
			}

			if (defHitFrames > 0) {
				hitFrames = defHitFrames;
			}
		} else {
			// kill the projectile
			if (owner->UseEnergy(weaponDef->shieldEnergyUse)) {
				if (weaponDef->shieldPower != 0) {
					curPower -= pro->weaponDef->damages[0];
				}

				pro->Collision(owner);

				if (defHitFrames > 0) {
					hitFrames = defHitFrames;
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

	const float closeLength = std::max(0.0f, dif.dot(dir));
	const float3 closeVect = dif - dir * closeLength;
	const float closeDist = closeVect.SqLength2D();

	// TODO: this isn't good enough in case shield is mounted on a mobile unit,
	//       and this unit moves relatively fast compared to the projectile.
	// it should probably be: radius + closeLength / |projectile->speed| * |owner->speed|,
	// but this still doesn't solve anything for e.g. teleporting shields.
	if (closeDist < Square(radius * 1.5f)) {
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
