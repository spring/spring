/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "DamageArray.h"
#include "DamageArrayHandler.h"

#include "System/float3.h"

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
	, refCount(1)
{ }


DamageArray DynDamageArray::GetDynamicDamages(const float3& startPos, const float3& curPos) const
{
	DamageArray dynDamages(*this);

	if (dynDamageExp <= 0.0f)
		return dynDamages;


	const float travDist  = std::min(dynDamageRange, curPos.distance2D(startPos));
	const float damageMod = 1.0f - math::pow(1.0f / dynDamageRange * travDist, dynDamageExp);
	const float ddmod     = dynDamageMin / damages[0]; // get damage mod from first damage type

	if (dynDamageInverted) {
		for (int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damages[i] - damageMod * damages[i];

			if (dynDamageMin > 0.0f)
				dynDamages[i] = std::max(damages[i] * ddmod, dynDamages[i]);

			// to prevent div by 0
			dynDamages[i] = std::max(0.0001f, dynDamages[i]);
		}
	} else {
		for (int i = 0; i < damageArrayHandler->GetNumTypes(); ++i) {
			dynDamages[i] = damageMod * damages[i];

			if (dynDamageMin > 0.0f)
				dynDamages[i] = std::max(damages[i] * ddmod, dynDamages[i]);

			// div by 0
			dynDamages[i] = std::max(0.0001f, dynDamages[i]);
		}
	}

	return dynDamages;
}


void DynDamageArray::Duplicate(DynDamageArray*& dda)
{
	if (dda->refCount == 1)
		return;

	DecRef(dda);

	dda = new DynDamageArray(*dda);
}
