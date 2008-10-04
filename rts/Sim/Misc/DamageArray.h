#ifndef __DAMAGE_ARRAY_H__
#define __DAMAGE_ARRAY_H__

#include <algorithm>
#include "creg/creg.h"

struct DamageArray
{
	CR_DECLARE_STRUCT(DamageArray);

public:

	DamageArray();
	DamageArray(const DamageArray& other);
	~DamageArray();

	void operator=(const DamageArray& other) {
		paralyzeDamageTime = other.paralyzeDamageTime;
		impulseFactor = other.impulseFactor;
		impulseBoost = other.impulseBoost;
		craterMult = other.craterMult;
		craterBoost = other.craterBoost;
		numTypes = other.numTypes;
		std::copy(other.damages, other.damages + numTypes, damages);
	}
	float& operator[](int i) { return damages[i]; }
	float operator[](int i) const { return damages[i]; }

	DamageArray operator*(float mul) const {
		DamageArray da(*this);
		for(int a = 0; a < numTypes; ++a)
			da.damages[a] *= mul;
		return da;
	}

	float GetDefaultDamage() const { return damages[0]; }

	int paralyzeDamageTime;
	float impulseFactor, impulseBoost, craterMult, craterBoost;

private:
	void creg_Serialize(creg::ISerializer& s);

	int numTypes;
	float* damages;
};

#endif // __DAMAGE_ARRAY_H__
