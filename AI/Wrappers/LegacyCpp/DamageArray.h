/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _DAMAGE_ARRAY_H_
#define _DAMAGE_ARRAY_H_

#include <algorithm>
#include <vector>
#include "System/creg/creg_cond.h"


namespace springLegacyAI {

struct DamageArray
{
	CR_DECLARE_STRUCT(DamageArray)

public:

	DamageArray();
	DamageArray(const float mul);
	/**
	 * This constructor is currently only used by C++ AIs
	 * which use the legacy C++ wrapper around the C AI interface.
	 */
	DamageArray(const std::vector<float>& dmg);
	DamageArray(const DamageArray& other);

	DamageArray& operator=(const DamageArray& other) {
		paralyzeDamageTime = other.paralyzeDamageTime;
		impulseFactor = other.impulseFactor;
		impulseBoost = other.impulseBoost;
		craterMult = other.craterMult;
		craterBoost = other.craterBoost;
		damages = other.damages;
		return *this;
	}
	float& operator[](int i) { return damages[i]; }
	float operator[](int i) const { return damages[i]; }

	DamageArray operator*(float mul) const {
		DamageArray da(*this);
		for (int a = 0; a < damages.size(); ++a)
			da.damages[a] *= mul;
		return da;
	}

	int GetNumTypes() const { return damages.size(); }
	float GetTypeDamage(int typeIndex) const { return damages[typeIndex]; }
	float GetDefaultDamage() const { return damages[0]; }

	int paralyzeDamageTime;
	float impulseFactor, impulseBoost, craterMult, craterBoost;

private:
	#ifdef USING_CREG
	void creg_Serialize(creg::ISerializer* s);
	#endif // USING_CREG

	std::vector<float> damages;
};

} // namespace springLegacyAI

#endif // _DAMAGE_ARRAY_H_
