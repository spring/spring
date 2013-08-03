/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/STL_List.h"
#include "System/creg/STL_Set.h"
#include "PlasmaRepulser.h"
#include "Lua/LuaRules.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/InterceptHandler.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Sim/Projectiles/Unsynced/ShieldProjectile.h"
#include "Sim/Projectiles/Unsynced/RepulseGfx.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/myMath.h"

CR_BIND_DERIVED(CPlasmaRepulser, CWeapon, (NULL));

CR_REG_METADATA(CPlasmaRepulser, (
	CR_MEMBER(radius),
	CR_MEMBER(sqRadius),
	CR_MEMBER(curPower),
	CR_MEMBER(hitFrames),
	CR_MEMBER(rechargeDelay),
	CR_MEMBER(isEnabled),
	CR_MEMBER(shieldProjectile),
	CR_MEMBER(hasGfx)
));


CPlasmaRepulser::CPlasmaRepulser(CUnit* owner)
: CWeapon(owner),
	curPower(0),
	radius(0),
	sqRadius(0),
	hitFrames(0),
	rechargeDelay(0),
	isEnabled(true)
{
	interceptHandler.AddPlasmaRepulser(this);
}


CPlasmaRepulser::~CPlasmaRepulser()
{
	interceptHandler.RemovePlasmaRepulser(this);
	shieldProjectile->PreDelete();
}


void CPlasmaRepulser::Init()
{
	radius = weaponDef->shieldRadius;
	sqRadius = radius * radius;

	if (weaponDef->shieldPower == 0)
		curPower = 99999999999.0f;
	else
		curPower = weaponDef->shieldStartingPower;

	CWeapon::Init();

	// deleted by ProjectileHandler
	shieldProjectile = new ShieldProjectile(this);
}


bool CPlasmaRepulser::HaveFreeLineOfFire(const float3& pos, bool userTarget, const CUnit* unit) const
{
	return true;
}


void CPlasmaRepulser::Update()
{
	const int defHitFrames = weaponDef->visibleShieldHitFrames;
	const int defRechargeDelay = weaponDef->shieldRechargeDelay;

	rechargeDelay -= (rechargeDelay > 0) ? 1 : 0;

	if (isEnabled && (curPower < weaponDef->shieldPower) && rechargeDelay <= 0) {
		if (owner->UseEnergy(weaponDef->shieldPowerRegenEnergy * (1.0f / GAME_SPEED))) {
			curPower += weaponDef->shieldPowerRegen * (1.0f / GAME_SPEED);
		}
	}

	if (hitFrames > 0)
		hitFrames--;

	weaponPos = owner->pos + (owner->frontdir * relWeaponPos.z)
	                       + (owner->updir    * relWeaponPos.y)
	                       + (owner->rightdir * relWeaponPos.x);

	if (!isEnabled) {
		return;
	}

	for (std::map<int, CWeaponProjectile*>::iterator pi = incomingProjectiles.begin(); pi != incomingProjectiles.end(); ++pi) {
		assert(projectileHandler->GetMapPairBySyncedID(pi->first)); // valid projectile id?

		CWeaponProjectile* pro = pi->second;
		const WeaponDef* proWD = pro->GetWeaponDef();

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

		if (curPower < proWD->damages[0]) {
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
					//FIXME some weapons do range dependent damage! (mantis #2345)
					curPower -= proWD->damages[0];
				}
			} else {
				//FIXME why do all weapons except LASERs do only (1 / GAME_SPEED) damage???
				owner->UseEnergy(weaponDef->shieldEnergyUse / GAME_SPEED);

				if (weaponDef->shieldPower != 0) {
					curPower -= proWD->damages[0] / GAME_SPEED;
				}
			}

			if (weaponDef->visibleShieldRepulse) {
				bool newlyAdded = hasGfx.insert(pro).second;

				if (newlyAdded) {
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
					//FIXME some weapons do range dependent damage! (mantis #2345)
					curPower -= proWD->damages[0];
				}

				pro->Collision(owner);

				if (defHitFrames > 0) {
					hitFrames = defHitFrames;
				}
			}
		}
	}

}


void CPlasmaRepulser::SlowUpdate()
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
	// PlasmaRepulser instances are created if type == "Shield",
	// but projectiles are removed from incomingProjectiles only
	// if def->interceptor || def->isShield --> dangling pointers
	// due to isShield being a separate tag (in 91.0)
	assert(weaponDef->isShield);

	if (weaponDef->smartShield && p->owner() && teamHandler->AlliedTeams(p->owner()->team, owner->team)) {
		return;
	}

	float3 dir = p->speed;
	if (p->GetTargetPos() != ZeroVector) {
		// assume projectile will travel roughly in the direction of its targetpos
		dir = p->GetTargetPos() - p->pos;
	}

	dir.y = 0.0f;
	dir.SafeNormalize();

	const float3 dif = owner->pos - p->pos;

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
		incomingProjectiles[p->id] = p;
		AddDeathDependence(p, DEPENDENCE_REPULSED);
	}
}


float CPlasmaRepulser::NewBeam(CWeapon* emitter, float3 start, float3 dir, float length, float3& newDir)
{
	if (!isEnabled) {
		return -1.0f;
	}

	// ::Update handles projectile <-> shield interactions for normal weapons, but
	// does not get called when the owner is stunned (CUnit::Update returns early)
	// BeamLasers and LightningCannons are hitscan (their projectiles do not move),
	// they call InterceptHandler to figure out if a shield is in the way so check
	// the stunned-state here keep things consistent
	if (owner->IsStunned() || owner->beingBuilt) {
		return -1.0f;
	}

	if (emitter->weaponDef->damages[0] > curPower) {
		return -1.0f;
	}
	if (weaponDef->smartShield && teamHandler->AlliedTeams(emitter->owner->team, owner->team)) {
		return -1.0f;
	}

	const float3 dif = weaponPos - start;

	if (weaponDef->exteriorShield && dif.SqLength() < sqRadius) {
		return -1.0f;
	}

	const float closeLength = dif.dot(dir);

	if (closeLength < 0.0f) {
		return -1.0f;
	}

	const float3 closeVect = dif - dir * closeLength;

	const float tmp = sqRadius - closeVect.SqLength();
	if ((tmp > 0.0f) && (length > (closeLength - math::sqrt(tmp)))) {
		const float colLength = closeLength - math::sqrt(tmp);
		const float3 colPoint = start + dir * colLength;
		const float3 normal = (colPoint - weaponPos).Normalize();
		newDir = dir - normal * normal.dot(dir) * 2;
		return colLength;
	}
	return -1.0f;
}


void CPlasmaRepulser::DependentDied(CObject* o)
{
	hasGfx.erase(static_cast<CWeaponProjectile*>(o));
	CWeapon::DependentDied(o);
}


bool CPlasmaRepulser::BeamIntercepted(CWeapon* emitter, float damageMultiplier)
{
	if (weaponDef->shieldPower > 0) {
		//FIXME some weapons do range dependent damage! (mantis #2345)
		curPower -= emitter->weaponDef->damages[0] * damageMultiplier;
	}
	return weaponDef->shieldRepulser;
}
