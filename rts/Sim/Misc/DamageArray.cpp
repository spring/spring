/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */
#include "DamageArray.h"
#include "DamageArrayHandler.h"

#include "System/float3.h"

#include <cassert>

CR_BIND(DamageArray, )

CR_REG_METADATA(DamageArray, (
	CR_MEMBER(paralyzeDamageTime),
	CR_MEMBER(impulseFactor),
	CR_MEMBER(impulseBoost),
	CR_MEMBER(craterMult),
	CR_MEMBER(craterBoost),
	CR_MEMBER(damages)
))

void DamageArray::SetDefaultDamage(float damage)
{
	damages.clear();
	damages.resize(std::max(1, damageArrayHandler.GetNumTypes()), damage);
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
	CR_MEMBER(refCount),
	CR_MEMBER(fromDef),

	CR_POSTLOAD(PostLoad)
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
	, fromDef(false)
{ }

DynDamageArray::~DynDamageArray()
{
	assert(refCount == 1);
}

void DynDamageArray::PostLoad()
{
	// weapondefs aren't serialized but if their damages tables are in use
	// these pointers will be serialized.
	// so during loading, a duplicate of the damages table is created
	// with a wrong refCount (it still counts the ref in the weaponDef).
	// that's why we need to decrement it and mark that we aren't a def table
	if (fromDef) {
		--refCount;
		fromDef = false;
	}
	assert(refCount > 0);
}

DamageArray DynDamageArray::GetDynamicDamages(const float3& startPos, const float3& curPos) const
{
	DamageArray dynDamages(*this);

	if (dynDamageExp <= 0.0f)
		return dynDamages;


	const float travDist  = std::min(dynDamageRange, curPos.distance2D(startPos));
	const float damageMod = 1.0f - math::pow(1.0f / dynDamageRange * travDist, dynDamageExp);
	const float ddmod     = dynDamageMin / damages[0]; // get damage mod from first damage type

	if (dynDamageInverted) {
		for (int i = 0; i < damageArrayHandler.GetNumTypes(); ++i) {
			float d = damages[i] - damageMod * damages[i];

			if (dynDamageMin > 0.0f)
				d = std::max(damages[i] * ddmod, d);

			// to prevent div by 0
			d = std::max(0.0001f, d);
			dynDamages.Set(i, d);
		}
	} else {
		for (int i = 0; i < damageArrayHandler.GetNumTypes(); ++i) {
			float d = damageMod * damages[i];

			if (dynDamageMin > 0.0f)
				d = std::max(damages[i] * ddmod, d);

			// div by 0
			d = std::max(0.0001f, d);
			dynDamages.Set(i, d);
		}
	}

	return dynDamages;
}


const DynDamageArray* DynDamageArray::IncRef(const DynDamageArray* dda)
{
	++dda->refCount;
	return dda;
}


void DynDamageArray::DecRef(const DynDamageArray* dda)
{
	if (dda->refCount == 1) {
		delete const_cast<DynDamageArray*>(dda);
	} else {
		--dda->refCount;
	}
}

DynDamageArray* DynDamageArray::GetMutable(const DynDamageArray*& dda)
{
	if (dda != nullptr) {
		if (dda->refCount == 1)
			return const_cast<DynDamageArray*>(dda);

		//We're still in use by someone, so copy and replace
		//pointer
		DecRef(dda);
	}

	DynDamageArray* newDDA = new DynDamageArray(*dda);
	dda = newDDA;
	return newDDA;
}
