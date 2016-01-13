/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DamageArray.h"
#include "DamageArrayHandler.h"

CR_BIND(DamageArray, )

CR_REG_METADATA(DamageArray, (
	CR_MEMBER(paralyzeDamageTime),
	CR_MEMBER(impulseFactor),
	CR_MEMBER(impulseBoost),
	CR_MEMBER(craterMult),
	CR_MEMBER(craterBoost),
	CR_MEMBER(damages)
))

DamageArray::DamageArray(float damage)
	: paralyzeDamageTime(0)
	, impulseFactor(1.0f)
	, impulseBoost(0.0f)
	, craterMult(1.0f)
	, craterBoost(0.0f)
{
	SetDefaultDamage(damage);
}


DamageArray& DamageArray::operator = (const DamageArray& other) {
	paralyzeDamageTime = other.paralyzeDamageTime;
	impulseFactor = other.impulseFactor;
	impulseBoost = other.impulseBoost;
	craterMult = other.craterMult;
	craterBoost = other.craterBoost;
	damages = other.damages;
	return *this;
}

DamageArray DamageArray::operator * (float damageMult) const {
	DamageArray da(*this);

	for (unsigned int a = 0; a < damages.size(); ++a)
		da.damages[a] *= damageMult;

	return da;
}


void DamageArray::SetDefaultDamage(float damage)
{
	damages.clear();
	if (damageArrayHandler != NULL) {
		damages.resize(damageArrayHandler->GetNumTypes(), damage);
		assert(!damages.empty());
	} else {
		// default-damage only (never reached?)
		damages.resize(1, damage);
	}
}


CR_BIND_DERIVED(DynDamageArray, DamageArray, )

CR_REG_METADATA(DynDamageArray, (
	CR_MEMBER(dynDamageExp),
	CR_MEMBER(dynDamageMin),
	CR_MEMBER(dynDamageRange),
	CR_MEMBER(dynDamageInverted),
	CR_MEMBER(craterAreaOfEffect),
	CR_MEMBER(damageAreaOfEffect),
	CR_MEMBER(edgeEffectiveness),
	CR_MEMBER(explosionSpeed),
	CR_MEMBER(refCount)
))

DynDamageArray::DynDamageArray(float damage)
	: DamageArray(damage)
	, dynDamageExp(0.0f)
	, dynDamageMin(0.0f)
	, dynDamageRange(0.0f)
	, dynDamageInverted(false)
	, craterAreaOfEffect(4.0f)
	, damageAreaOfEffect(4.0f)
	, edgeEffectiveness(0.0f)
	, explosionSpeed(1.0f) // always overwritten
	, refCount(0)
{ }
