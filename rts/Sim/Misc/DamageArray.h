#ifndef __DAMAGE_ARRAY_H__
#define __DAMAGE_ARRAY_H__

#include "StdAfx.h"
#include <algorithm>
#include "creg/creg.h"

struct DamageArray
{
	CR_DECLARE_STRUCT(DamageArray);

public:

	DamageArray();

	/**
	 * This constructor is currently only used by C++ AIs
	 * which use the legacy C++ wrapper around the C AI interface.
	 */
	DamageArray(int numTypes, const float* typeDamages) :
			numTypes(numTypes)
	{
		damages = new float[numTypes];
		for(int a = 0; a < numTypes; ++a) {
			damages[a] = typeDamages[a];
		}
	}

DamageArray(const DamageArray& other)
{
	paralyzeDamageTime = other.paralyzeDamageTime;
	impulseBoost = other.impulseBoost;
	craterBoost = other.craterBoost;
	impulseFactor = other.impulseFactor;
	craterMult = other.craterMult;
	numTypes = other.numTypes;
	damages = SAFE_NEW float[numTypes];
	for(int a = 0; a < numTypes; ++a)
		damages[a] = other.damages[a];
}


	~DamageArray()
	{
		delete[] damages;
	}

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

	int GetNumTypes() const { return numTypes; }
	float GetTypeDamage(int typeIndex) const { return damages[typeIndex]; }
	float GetDefaultDamage() const { return damages[0]; }

	int paralyzeDamageTime;
	float impulseFactor, impulseBoost, craterMult, craterBoost;

private:
	void creg_Serialize(creg::ISerializer& s);

	int numTypes;
	float* damages;
};

#endif // __DAMAGE_ARRAY_H__
