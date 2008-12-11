
#include "DamageArray.h"

#include "Sync/Syncify.h"
#if defined __cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
#include "DamageArrayHandler.h"
#endif // __cplusplus && !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE


#ifdef USING_CREG
CR_BIND(DamageArray, );

CR_REG_METADATA(DamageArray, (
		CR_MEMBER(paralyzeDamageTime),
		CR_MEMBER(impulseFactor),
		CR_MEMBER(impulseBoost),
		CR_MEMBER(craterMult),
		CR_MEMBER(craterBoost),
		CR_MEMBER(numTypes),
		CR_RESERVED(16),
		CR_SERIALIZER(creg_Serialize) // damages
));


void DamageArray::creg_Serialize(creg::ISerializer& s)
{
	s.Serialize(damages, numTypes * sizeof(damages[0]));
}
#endif // USING_CREG

DamageArray::DamageArray() : paralyzeDamageTime(0),
			impulseFactor(1.0f), impulseBoost(0.0f),
			craterMult(1.0f), craterBoost(0.0f)
{
#if !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
	if (damageArrayHandler) {
		numTypes = damageArrayHandler->GetNumTypes();
	} else {
		numTypes = 1;
	}
#else // !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
	numTypes = 1;
#endif // !defined BUILDING_AI && !defined BUILDING_AI_INTERFACE
	damages = SAFE_NEW float[numTypes];
	for(int a = 0; a < numTypes; ++a) {
		damages[a] = 1.0f;
	}
}

DamageArray::DamageArray(int numTypes, const float* typeDamages) :
			numTypes(numTypes)
{
	damages = new float[numTypes];
	for(int a = 0; a < numTypes; ++a) {
		damages[a] = typeDamages[a];
	}
}

DamageArray::DamageArray(const DamageArray& other)
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

DamageArray::~DamageArray()
{
	delete[] damages;
}
