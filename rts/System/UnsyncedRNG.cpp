/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "System/UnsyncedRNG.h"
#include "Sim/Misc/GlobalConstants.h"

#include <limits.h>

// like 5-6x slower than our, but much better distribution
//#define USE_BOOST_RNG

#ifdef USE_BOOST_RNG
	#include <boost/random/mersenne_twister.hpp>
	#include <boost/random/uniform_01.hpp>
	#include <boost/random/uniform_smallint.hpp>
	#include <boost/random/uniform_on_sphere.hpp>
	#include <boost/random/variate_generator.hpp>

	static boost::random::mt19937 rng;
	//static boost::random::mt11213b rng;
	static boost::random::uniform_01<float> dist01;
	static boost::random::uniform_smallint<int> distInt(0, RANDINT_MAX);
	static boost::random::uniform_on_sphere<float> distSphere(3);
	static boost::variate_generator<boost::random::mt19937&, boost::uniform_01<float> > gen01(rng, dist01);
	static boost::variate_generator<boost::random::mt19937&, boost::uniform_smallint<int> > genInt(rng, distInt);
	static boost::variate_generator<boost::random::mt19937&, boost::uniform_on_sphere<float> > genSphere(rng, distSphere);
#endif


UnsyncedRNG::UnsyncedRNG() : randSeed(0)
{

}

void UnsyncedRNG::Seed(unsigned seed)
{
#ifdef USE_BOOST_RNG
	rng.seed(seed);
#else
	randSeed ^= seed;
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

int UnsyncedRNG::operator()(int n)
{
	return RandInt() * n / ((INT_MAX & RANDINT_MAX) + 1); // the range of RandInt() is limited to (INT_MAX & 0x7FFF)
}
