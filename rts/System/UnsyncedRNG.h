/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCEDRNG_H_
#define UNSYNCEDRNG_H_

#include "System/float3.h"

// like 5-6x slower than our, but much better distribution
//#define USE_BOOST_RNG
#ifdef USE_BOOST_RNG
	#include <boost/random/mersenne_twister.hpp>
	#include <boost/random/uniform_01.hpp>
	#include <boost/random/uniform_smallint.hpp>
	#include <boost/random/uniform_on_sphere.hpp>
	#include <boost/random/variate_generator.hpp>
#endif


class UnsyncedRNG
{
public:
	UnsyncedRNG();
	void Seed(unsigned seed);
	
	/** @brief returns a random unsigned integer in the range [0, (UINT_MAX & 0x7FFF)) */
	unsigned operator()() { int r = RandInt(); return *reinterpret_cast<unsigned*>(&r); }
	
	/** @brief returns a random integer in the range [0, (INT_MAX & 0x7FFF)) */
	int RandInt();

	/** @brief returns a random float in the range [0, 1) */
	float RandFloat();
	
	/** @brief returns a random vector with length [0, 1] */
	float3 RandVector();
	float3 RandVector2D();
	
	/** @brief returns a random number in the range [0, n) */
	int operator()(int n);
	void operator=(const UnsyncedRNG& urng);

private:
	unsigned randSeed;

#ifdef USE_BOOST_RNG
	boost::random::mt19937 rng;
	boost::random::uniform_01<float> dist01;
	boost::random::uniform_smallint<int> distInt;
	boost::random::uniform_on_sphere<float> distSphere;
	boost::variate_generator<boost::random::mt19937&, boost::uniform_01<float> > gen01;
	boost::variate_generator<boost::random::mt19937&, boost::uniform_smallint<int> > genInt;
	boost::variate_generator<boost::random::mt19937&, boost::uniform_on_sphere<float> > genSphere;
#endif
};

#endif // UNSYNCEDRNG_H_

