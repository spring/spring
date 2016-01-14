/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DAMAGE_ARRAY_H
#define DAMAGE_ARRAY_H

#include <algorithm>
#include <vector>
#include "System/creg/creg_cond.h"

struct float3;

class DamageArray
{
	CR_DECLARE(DamageArray)

public:
	DamageArray(float damage = 1.0f);
	DamageArray(const DamageArray& other) { *this = other; }
	virtual ~DamageArray() { }

	DamageArray operator * (float damageMult) const;




	void SetDefaultDamage(float damage);

	int GetNumTypes() const { return damages.size(); }
	float Get(int typeIndex) const { return damages[typeIndex]; }
	void Set(int typeIndex, float d) { damages[typeIndex] = d; }
	float GetDefault() const { return damages[0]; }

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
	DynDamageArray(const DynDamageArray& other) { *this = other; refCount = 1; }
	~DynDamageArray();

	DamageArray GetDynamicDamages(const float3& startPos, const float3& curPos) const;

	static DynDamageArray* IncRef(DynDamageArray* dda) { ++dda->refCount; return dda; };
	static void DecRef(DynDamageArray* dda) { if (dda->refCount == 1) delete dda; else --dda->refCount; };
	static void Duplicate(DynDamageArray*& dda);

	float dynDamageExp;
	float dynDamageMin;
	float dynDamageRange;
	bool dynDamageInverted;
	float craterAreaOfEffect;
	float damageAreaOfEffect;
	float edgeEffectiveness;
	float explosionSpeed;
	int refCount;
};

#endif
