/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "DamageArray.h"

#include "System/creg/creg_cond.h"

#ifdef USING_CREG
namespace springLegacyAI {

CR_BIND(DamageArray, )

CR_REG_METADATA(DamageArray, (
	CR_MEMBER(paralyzeDamageTime),
	CR_MEMBER(impulseFactor),
	CR_MEMBER(impulseBoost),
	CR_MEMBER(craterMult),
	CR_MEMBER(craterBoost),
	CR_SERIALIZER(creg_Serialize) // damages
))

} // namespace springLegacyAI

void springLegacyAI::DamageArray::creg_Serialize(creg::ISerializer* s)
{
	s->Serialize(&damages[0], damages.size() * sizeof(damages[0]));
}
#endif // USING_CREG

springLegacyAI::DamageArray::DamageArray():
	paralyzeDamageTime(0),
	impulseFactor(1.0f),
	impulseBoost(0.0f),
	craterMult(1.0f),
	craterBoost(0.0f)
{
	damages.resize(1, 1.0f);
}

springLegacyAI::DamageArray::DamageArray(const float mult):
	paralyzeDamageTime(0),
	impulseFactor(1.0f),
	impulseBoost(0.0f),
	craterMult(1.0f),
	craterBoost(0.0f)
{
	damages.resize(1, mult);
}

springLegacyAI::DamageArray::DamageArray(const std::vector<float>& dmg)
	: paralyzeDamageTime(0)
	, impulseFactor(1.0f)
	, impulseBoost(0.0f)
	, craterMult(1.0f)
	, craterBoost(0.0f)
{
	damages = dmg;
}

springLegacyAI::DamageArray::DamageArray(const springLegacyAI::DamageArray& other)
{
	paralyzeDamageTime = other.paralyzeDamageTime;
	impulseBoost = other.impulseBoost;
	craterBoost = other.craterBoost;
	impulseFactor = other.impulseFactor;
	craterMult = other.craterMult;
	damages = other.damages;
}

