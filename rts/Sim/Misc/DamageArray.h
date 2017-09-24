/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DAMAGE_ARRAY_H
#define DAMAGE_ARRAY_H

#include <algorithm>
#include <vector>
#include "System/creg/creg_cond.h"

class float3;

class DamageArray
{
	CR_DECLARE(DamageArray)

public:
	DamageArray(float damage = 1.0f)
		: paralyzeDamageTime(0)

		, impulseFactor(1.0f)
		, impulseBoost(0.0f)

		, craterMult(1.0f)
		, craterBoost(0.0f)
	{
		SetDefaultDamage(damage);
	}


	DamageArray(const DamageArray& da) { *this = da; }
	DamageArray(DamageArray&& da) { *this = std::move(da); }

	virtual ~DamageArray() {}

	DamageArray& operator = (const DamageArray& da) {
		paralyzeDamageTime = da.paralyzeDamageTime;

		impulseFactor = da.impulseFactor;
		impulseBoost = da.impulseBoost;

		craterMult = da.craterMult;
		craterBoost = da.craterBoost;

		damages = da.damages;
		return *this;
	}
	DamageArray& operator = (DamageArray&& da) {
		paralyzeDamageTime = da.paralyzeDamageTime;

		impulseFactor = da.impulseFactor;
		impulseBoost = da.impulseBoost;

		craterMult = da.craterMult;
		craterBoost = da.craterBoost;

		damages = std::move(da.damages);
		return *this;
	}

	DamageArray operator * (float damageMult) const {
		DamageArray da(*this);

		for (unsigned int a = 0; a < damages.size(); ++a)
			da.damages[a] *= damageMult;

		return da;
	}


	int GetNumTypes() const { return damages.size(); }

	void SetDefaultDamage(float damage);
	void Set(int typeIndex, float d) { damages[typeIndex] = d; }

	float Get(int typeIndex) const { return damages[typeIndex]; }
	float GetDefault() const { return damages[0]; }

public:
	int paralyzeDamageTime;

	float impulseFactor;
	float impulseBoost;

	float craterMult;
	float craterBoost;

protected:
	std::vector<float> damages;
};


class DynDamageArray : public DamageArray
{
	CR_DECLARE(DynDamageArray)

public:
	DynDamageArray(float damage = 1.0f);
	DynDamageArray(const DynDamageArray& da) { *this = da; fromDef = false; refCount = 1; }
	~DynDamageArray();

	void PostLoad();

	DamageArray GetDynamicDamages(const float3& startPos, const float3& curPos) const;

	static const DynDamageArray* IncRef(const DynDamageArray* dda);
	static void DecRef(const DynDamageArray* dda);
	static DynDamageArray* GetMutable(const DynDamageArray*& dda);

	float dynDamageExp;
	float dynDamageMin;
	float dynDamageRange;
	bool dynDamageInverted;
	float craterAreaOfEffect;
	float damageAreaOfEffect;
	float edgeEffectiveness;
	float explosionSpeed;
	mutable int refCount;
	bool fromDef;
};

#endif
