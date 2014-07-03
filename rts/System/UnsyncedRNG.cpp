/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/UnsyncedRNG.h"
#include "Sim/Misc/GlobalConstants.h"

#include <limits.h>



UnsyncedRNG::UnsyncedRNG()
: randSeed(0)
#ifdef USE_BOOST_RNG
, distInt(0, RANDINT_MAX)
, distSphere(3)
, gen01(rng, dist01)
, genInt(rng, distInt)
, genSphere(rng, distSphere)
#endif
{

}


void UnsyncedRNG::operator=(const UnsyncedRNG& urng)
{
	randSeed = urng.randSeed;
	Seed(randSeed);
}


void UnsyncedRNG::Seed(unsigned seed)
{
	randSeed = seed;
#ifdef USE_BOOST_RNG
	rng.seed(seed);
#endif
}

int UnsyncedRNG::RandInt()
{
#ifdef USE_BOOST_RNG
	return genInt();
#else
	randSeed = (randSeed * 214013L + 2531011L);
	return (randSeed >> 16) & RANDINT_MAX;
#endif
}

float UnsyncedRNG::RandFloat()
{
#ifdef USE_BOOST_RNG
	return gen01();
#else
	randSeed = (randSeed * 214013L + 2531011L);
	return float((randSeed >> 16) & RANDINT_MAX) / RANDINT_MAX;
#endif
}

float3 UnsyncedRNG::RandVector()
{
#ifdef USE_BOOST_RNG
	float3 ret(&genSphere()[0]);
	ret *= RandFloat();
#else
	float3 ret;
	do {
		ret.x = RandFloat() * 2 - 1;
		ret.y = RandFloat() * 2 - 1;
		ret.z = RandFloat() * 2 - 1;
	} while (ret.SqLength() > 1);
#endif

	return ret;
}

float3 UnsyncedRNG::RandVector2D()
{
	float3 ret;
	do {
		ret.x = RandFloat() * 2 - 1;
		ret.z = RandFloat() * 2 - 1;
	} while (ret.SqLength() > 1);

	return ret;
}

int UnsyncedRNG::operator()(int n)
{
	return RandInt() * n / ((INT_MAX & RANDINT_MAX) + 1); // the range of RandInt() is limited to (INT_MAX & 0x7FFF)
}
