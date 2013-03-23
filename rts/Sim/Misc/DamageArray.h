/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef DAMAGE_ARRAY_H
#define DAMAGE_ARRAY_H

#include <algorithm>
#include <vector>
#include "System/creg/creg_cond.h"

struct DamageArray
{
	CR_DECLARE_STRUCT(DamageArray);

public:
	DamageArray(float damage = 1.0f);
	DamageArray(const DamageArray& other) { *this = other; }
	~DamageArray() { }

	DamageArray& operator = (const DamageArray& other);
	DamageArray operator * (float damageMult) const;

	float& operator [] (int i)       { return damages.at(i); }
	float  operator [] (int i) const { return damages.at(i); }

	void SetDefaultDamage(float damage);

	int GetNumTypes() const { return damages.size(); }
	float GetTypeDamage(int typeIndex) const { return damages.at(typeIndex); }
	float GetDefaultDamage() const { return damages.at(0); }

	int paralyzeDamageTime;
	float
		impulseFactor,
		impulseBoost,
		craterMult,
		craterBoost;

private:
	std::vector<float> damages;
};

#endif
