/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "PlasmaRepulser.h"
#include "Sim/Misc/GlobalSynced.h"
#include "Sim/Misc/QuadField.h"
#include "Sim/Misc/TeamHandler.h"
#include "Rendering/Env/Particles/Classes/ShieldSegmentProjectile.h"
#include "Rendering/Env/Particles/Classes/RepulseGfx.h"
#include "Sim/Projectiles/WeaponProjectiles/WeaponProjectile.h"
#include "Sim/Projectiles/ProjectileMemPool.h"
#include "Sim/Units/Scripts/UnitScript.h"
#include "Sim/Units/Unit.h"
#include "Sim/Weapons/WeaponDef.h"
#include "System/EventHandler.h"
#include "System/SpringMath.h"

CR_BIND_DERIVED(CPlasmaRepulser, CWeapon, )
CR_REG_METADATA(CPlasmaRepulser, (
	CR_MEMBER(tempNum),
	CR_MEMBER(scIndex),

	CR_MEMBER(hitFrameCount),
	CR_MEMBER(rechargeDelay),

	CR_MEMBER(radius),
	CR_MEMBER(sqRadius),
	CR_MEMBER(curPower),

	CR_MEMBER(lastMuzzlePos),
	CR_MEMBER(deltaMuzzlePos),

	CR_MEMBER(isEnabled),

	CR_MEMBER(repulsedProjectiles),
	CR_MEMBER(quads),

	CR_MEMBER(collisionVolume)
))



struct ShieldSegmentCollectionPool {
	CR_DECLARE_STRUCT(ShieldSegmentCollectionPool)
public:
	void InsertCollection(CPlasmaRepulser* r) {
		if (sscs.empty())
			sscs.reserve(32);

		if (idcs.empty()) {
			sscs.emplace_back();

			sscs[r->scIndex = sscs.size() - 1].Init(r);
		} else {
			sscs[r->scIndex = idcs.back()].Init(r);

			idcs.pop_back();
		}
	}

	void RemoveCollection(const CPlasmaRepulser* r) {
		sscs[r->scIndex].Kill();
		idcs.push_back(r->scIndex);
	}

	void UpdateCollection(const CPlasmaRepulser* r) {
		sscs[r->scIndex].UpdateColor();
	}

private:
	// due to weapon pooling, ShieldSegmentCollection can not be
	// inlined without adding ~50 bytes to *all* weapon instances
	// (CPlasmaRepulser is the largest type)
	std::vector<unsigned int> idcs;
	std::vector<ShieldSegmentCollection> sscs;
};


CR_BIND(ShieldSegmentCollectionPool, )
CR_REG_METADATA(ShieldSegmentCollectionPool, (
	CR_MEMBER(idcs),
	CR_MEMBER(sscs)
))

static ShieldSegmentCollectionPool sscPool;

void CPlasmaRepulser::SerializeShieldSegmentCollectionPool(creg::ISerializer* s)
{
	s->SerializeObjectInstance(&sscPool, sscPool.GetClass());
}



CPlasmaRepulser::~CPlasmaRepulser()
{
	quadField.RemoveRepulser(this);
	sscPool.RemoveCollection(this);
}


void CPlasmaRepulser::Init()
{
	sqRadius = Square(radius = weaponDef->shieldRadius);
	curPower = mix(10.0f * 1000.0f * 1000.0f, weaponDef->shieldStartingPower, weaponDef->shieldPower != 0.0f);

	collisionVolume.InitSphere(radius);

	CWeapon::Init();

	sscPool.InsertCollection(this);
	quadField.MovedRepulser(this);
}


bool CPlasmaRepulser::IsRepulsing(CWeaponProjectile* p) const
{
	return (weaponDef->shieldRepulser && std::find(repulsedProjectiles.begin(), repulsedProjectiles.end(), p) != repulsedProjectiles.end());
}

bool CPlasmaRepulser::IgnoreInteriorHit(CWeaponProjectile* p) const
{
	return (weaponDef->exteriorShield && !IsRepulsing(p));
}


bool CPlasmaRepulser::IsActive() const
{
	return isEnabled && !owner->IsStunned() && !owner->beingBuilt;
}

bool CPlasmaRepulser::CanIntercept(unsigned interceptedType, int allyTeam) const
{
	if ((weaponDef->shieldInterceptType & interceptedType) == 0)
		return false;

	if (weaponDef->smartShield && teamHandler.IsValidAllyTeam(allyTeam) && teamHandler.AlliedAllyTeams(allyTeam, owner->allyteam))
		return false;

	return IsActive();
}


void CPlasmaRepulser::Update()
{
	rechargeDelay -= (rechargeDelay > 0);
	hitFrameCount -= (hitFrameCount > 0);

	if (IsActive() && (curPower < weaponDef->shieldPower) && rechargeDelay <= 0) {
		const float energyCost = weaponDef->shieldPowerRegenEnergy * (1.0f / GAME_SPEED);
		const float powerRegen = weaponDef->shieldPowerRegen       * (1.0f / GAME_SPEED);

		curPower += (powerRegen * owner->UseEnergy(energyCost));
	}

	UpdateWeaponVectors();

	if (weaponMuzzlePos != lastMuzzlePos)
		quadField.MovedRepulser(this);

	deltaMuzzlePos = mix(weaponMuzzlePos - lastMuzzlePos, deltaMuzzlePos, lastMuzzlePos == ZeroVector);
	lastMuzzlePos = weaponMuzzlePos;

	#if 0
	segmentCollections[this].UpdateColor();
	#endif
	sscPool.UpdateCollection(this);
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
	if (teamHandler.Team(owner->team)->res.energy < weaponDef->shieldEnergyUse)
		return false;

	rechargeDelay = defRechargeDelay;

	if (weaponDef->shieldRepulser) {
		// bounce the projectile
		switch (p->ShieldRepulse(weaponMuzzlePos, weaponDef->shieldForce, weaponDef->shieldMaxSpeed)) {
			case 0: { return false; } break;
			case 1: {
				owner->UseEnergy(weaponDef->shieldEnergyUse);

				curPower -= (shieldDamage * (weaponDef->shieldPower != 0.0f));
			} break;
			default: {
				// NOTE:
				//   all weapons except Lasers do only (1 / GAME_SPEED) damage
				//   (Lasers are insta-bounced, others spend time "inside" the
				//   shield dealing damage every frame)
				owner->UseEnergy(weaponDef->shieldEnergyUse / GAME_SPEED);

				curPower -= ((shieldDamage / GAME_SPEED) * (weaponDef->shieldPower != 0.0f));
			} break;
		}

		if (spring::VectorInsertUnique(repulsedProjectiles, p, true)) {
			// projectile was not repulsed before
			AddDeathDependence(p, DEPENDENCE_REPULSED);

			if (weaponDef->visibleShieldRepulse) {
				const float relPower = curPower / std::max(1.0f, weaponDef->shieldPower);
				const float lrpColor = std::min(1.0f, relPower);

				projMemPool.alloc<CRepulseGfx>(owner, p, radius, mix(weaponDef->shieldBadColor, weaponDef->shieldGoodColor, lrpColor));
			}
		}

		if (defHitFrames > 0)
			hitFrameCount = defHitFrames;

		return false;
	}

	// kill the projectile
	if (owner->UseEnergy(weaponDef->shieldEnergyUse)) {
		curPower -= (shieldDamage * (weaponDef->shieldPower != 0.0f));

		p->Collision();

		if (defHitFrames > 0)
			hitFrameCount = defHitFrames;

		return true;
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
	if (teamHandler.Team(owner->team)->res.energy < weaponDef->shieldEnergyUse)
		return false;

	curPower -= (shieldDamage * damageMultiplier * (weaponDef->shieldPower > 0.0f));
	return true;
}


void CPlasmaRepulser::DependentDied(CObject* o)
{
	spring::VectorErase(repulsedProjectiles, static_cast<CWeaponProjectile*>(o));
	CWeapon::DependentDied(o);
}
