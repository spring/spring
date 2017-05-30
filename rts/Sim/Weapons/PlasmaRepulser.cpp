/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/creg/STL_List.h"
#include "System/creg/STL_Set.h"
#include "PlasmaRepulser.h"
#include "WeaponMemPool.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Sim/Projectiles/ProjectileHandler.h"
#include "Rendering/Env/Particles/Classes/ShieldSegmentProjectile.h"
#include "Rendering/Env/Particles/Classes/RepulseGfx.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/myMath.h"

CR_BIND_DERIVED_POOL(CPlasmaRepulser, CWeapon, , weaponMemPool.alloc, weaponMemPool.free)
CR_REG_METADATA(CPlasmaRepulser, (
	CR_MEMBER(radius),
	CR_MEMBER(sqRadius),
	CR_MEMBER(lastPos),
	CR_MEMBER(curPower),
	CR_MEMBER(hitFrames),
	CR_MEMBER(rechargeDelay),
	CR_MEMBER(isEnabled),
	CR_MEMBER(segmentCollection),
	CR_MEMBER(repulsedProjectiles),

	CR_MEMBER(quads),
	CR_MEMBER(collisionVolume),
	CR_MEMBER(tempNum)
))


CPlasmaRepulser::CPlasmaRepulser(CUnit* owner, const WeaponDef* def): CWeapon(owner, def),
	tempNum(0),
	curPower(0.0f),
	hitFrames(0),
	rechargeDelay(0),
	isEnabled(true)
{ }


CPlasmaRepulser::~CPlasmaRepulser()
{
	delete segmentCollection;
	quadField->RemoveRepulser(this);
}


void CPlasmaRepulser::Init()
{
	collisionVolume.InitSphere(weaponDef->shieldRadius);

	radius = weaponDef->shieldRadius;
	sqRadius = radius * radius;

	if (weaponDef->shieldPower == 0)
		curPower = 99999999999.0f;
	else
		curPower = weaponDef->shieldStartingPower;

	CWeapon::Init();

	quadField->MovedRepulser(this);

	// deleted by ProjectileHandler
	segmentCollection = new ShieldSegmentCollection(this);
}


bool CPlasmaRepulser::IsRepulsing(CWeaponProjectile* p) const
{
	return weaponDef->shieldRepulser && std::find(repulsedProjectiles.begin(), repulsedProjectiles.end(), p) != repulsedProjectiles.end();
}

bool CPlasmaRepulser::IsActive() const
{
	return isEnabled && !owner->IsStunned() && !owner->beingBuilt;
}

bool CPlasmaRepulser::CanIntercept(unsigned interceptedType, int allyTeam) const
{
	if ((weaponDef->shieldInterceptType & interceptedType) == 0)
		return false;

	if (weaponDef->smartShield && teamHandler->IsValidAllyTeam(allyTeam) && teamHandler->AlliedAllyTeams(allyTeam, owner->allyteam))
		return false;

	return IsActive();
}


void CPlasmaRepulser::Update()
{
	rechargeDelay -= (rechargeDelay > 0) ? 1 : 0;

	if (IsActive() && (curPower < weaponDef->shieldPower) && rechargeDelay <= 0) {
		if (owner->UseEnergy(weaponDef->shieldPowerRegenEnergy * (1.0f / GAME_SPEED))) {
			curPower += weaponDef->shieldPowerRegen * (1.0f / GAME_SPEED);
		}
	}

	if (hitFrames > 0)
		hitFrames--;

	UpdateWeaponVectors();
	collisionVolume.SetOffsets(relWeaponMuzzlePos - owner->relMidPos);
	if (weaponMuzzlePos != lastPos)
		quadField->MovedRepulser(this);

	lastPos = weaponMuzzlePos;

	segmentCollection->UpdateColor();
}

// Returns true if the projectile is destroyed.
bool CPlasmaRepulser::IncomingProjectile(CWeaponProjectile* p, const float3& hitPos)
{
	const int defHitFrames = weaponDef->visibleShieldHitFrames;
	const int defRechargeDelay = weaponDef->shieldRechargeDelay;

	// gadget handles the collision event, don't touch the projectile
	// start-pos only makes sense for beams, pass current p->pos here
	if (eventHandler.ShieldPreDamaged(p, this, owner, weaponDef->shieldRepulser, nullptr, nullptr, p->pos, hitPos))
		return false;

	const DamageArray& damageArray = p->damages->GetDynamicDamages(p->GetStartPos(), p->pos);
	const float shieldDamage = damageArray.Get(weaponDef->shieldArmorType);

	// shield does not have enough power, don't touch the projectile
	if (curPower < shieldDamage)
		return false;

		// team does not have enough energy, don't touch the projectile
	if (teamHandler->Team(owner->team)->res.energy < weaponDef->shieldEnergyUse)
		return false;

	rechargeDelay = defRechargeDelay;

	if (weaponDef->shieldRepulser) {
		// bounce the projectile
		const int type = p->ShieldRepulse(weaponMuzzlePos,
			weaponDef->shieldForce,
			weaponDef->shieldMaxSpeed);

		if (type == 0) {
			return false;
		} else if (type == 1) {
			owner->UseEnergy(weaponDef->shieldEnergyUse);

			if (weaponDef->shieldPower != 0) {
				curPower -= shieldDamage;
			}
		} else {
			//FIXME why do all weapons except LASERs do only (1 / GAME_SPEED) damage???
			// because they go inside and take time to get pushed back
			// during that time they deal damage every frame
			// so in total they do their nominal damage each second
			// on the other hand lasers get insta-bounced in 1 frame
			// regardless of shield pushing power
			owner->UseEnergy(weaponDef->shieldEnergyUse / GAME_SPEED);

			curPower -= ((shieldDamage / GAME_SPEED) * (weaponDef->shieldPower != 0.0f));
		}

		if (spring::VectorInsertUnique(repulsedProjectiles, p, true)) {
			// projectile was not repulsed before
			AddDeathDependence(p, DEPENDENCE_REPULSED);

			if (weaponDef->visibleShieldRepulse) {
				const float colorMix = std::min(1.0f, curPower / std::max(1.0f, weaponDef->shieldPower));
				const float3 color =
					(weaponDef->shieldGoodColor * colorMix) +
					(weaponDef->shieldBadColor * (1.0f - colorMix));

				projMemPool.alloc<CRepulseGfx>(owner, p, radius, color);
			}
		}

		if (defHitFrames > 0)
			hitFrames = defHitFrames;

	} else {
		// kill the projectile
		if (owner->UseEnergy(weaponDef->shieldEnergyUse)) {
			curPower -= (shieldDamage * (weaponDef->shieldPower != 0.0f));

			p->Collision();

			if (defHitFrames > 0)
				hitFrames = defHitFrames;

			return true;
		}
	}

	return false;
}


void CPlasmaRepulser::SlowUpdate()
{
	UpdateWeaponPieces();
	UpdateWeaponVectors();
	owner->script->AimShieldWeapon(this);
}


bool CPlasmaRepulser::IncomingBeam(const CWeapon* emitter, const float3& startPos, const float3& hitPos, float damageMultiplier)
{
	// gadget handles the collision event, don't touch the projectile
	// note that startPos only equals the beam's true origin (which is
	// p->GetStartPos()) if it did not get reflected
	if (eventHandler.ShieldPreDamaged(nullptr, this, owner, weaponDef->shieldRepulser, emitter, emitter->owner, startPos, hitPos))
		return false;

	const DamageArray& damageArray = emitter->damages->GetDynamicDamages(startPos, weaponMuzzlePos);
	const float shieldDamage = damageArray.Get(weaponDef->shieldArmorType);

	if (curPower < shieldDamage)
		return false;

	// team does not have enough energy, don't touch the projectile
	if (teamHandler->Team(owner->team)->res.energy < weaponDef->shieldEnergyUse)
		return false;

	if (weaponDef->shieldPower > 0.0f)
		curPower -= shieldDamage * damageMultiplier;

	return true;
}


void CPlasmaRepulser::DependentDied(CObject* o)
{
	spring::VectorErase(repulsedProjectiles, static_cast<CWeaponProjectile*>(o));
	CWeapon::DependentDied(o);
}
