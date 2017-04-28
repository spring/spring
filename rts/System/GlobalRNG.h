/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RNG_H
#define _GLOBAL_RNG_H

#include <climits> // INT_MAX

#include "Sim/Misc/GlobalConstants.h" // RANDINT_MAX
#include "System/float3.h"

// crappy but fast LCG
class CGlobalSyncedRNG {
public:
	typedef unsigned int result_type;

	CGlobalSyncedRNG() { SetSeed(0, true); }

	// needed for std::{random_}shuffle
	result_type operator()(              ) { return (NextInt()    ); }
	result_type operator()(unsigned int N) { return (NextInt() % N); }

	result_type min() const { return           0; }
	result_type max() const { return RANDINT_MAX; }

	result_type NextInt() { return (((randSeed = (randSeed * 214013L + 2531011L)) >> 16) & RANDINT_MAX); }
	float NextFloat() { return ((NextInt() * 1.0f) / RANDINT_MAX); }

	float3 NextVector() {
		float3 ret;

		do {
			ret.x = NextFloat() * 2.0f - 1.0f;
			ret.y = NextFloat() * 2.0f - 1.0f;
			ret.z = NextFloat() * 2.0f - 1.0f;
		} while (ret.SqLength() > 1.0f);

		return ret;
	}

	unsigned int GetSeed() const { return randSeed; }
	unsigned int GetInitSeed() const { return initRandSeed; }

	void SetSeed(unsigned int seed, bool init) {
		randSeed = seed;
		initRandSeed = (seed * init) + initRandSeed * (1 - init);
	}

private:
	/**
	* @brief random seed
	*
	* Holds the synced random seed
	*/
	unsigned int randSeed;

	/**
	* @brief initial random seed
	*
	* Holds the synced initial random seed
	*/
	unsigned int initRandSeed;
};


class CGlobalUnsyncedRNG {
public:
	typedef unsigned int result_type;

	CGlobalUnsyncedRNG(): randSeed(0) {}

	void Seed(unsigned int seed) { randSeed = seed; }
	void operator = (const CGlobalUnsyncedRNG& urng) { Seed(urng.randSeed); }

	/** @brief returns a random integer in either the range [0, UINT_MAX & 0x7FFF) or in [0, n) */
	result_type operator()(              ) { return (NextInt()                                    ); }
	result_type operator()(unsigned int n) { return (NextInt() * n / ((INT_MAX & RANDINT_MAX) + 1)); }

	result_type min() const { return           0; }
	result_type max() const { return RANDINT_MAX; }

	/** @brief returns a random integer in the range [0, INT_MAX & 0x7FFF) */
	result_type NextInt() { return (((randSeed = (randSeed * 214013L + 2531011L)) >> 16) & RANDINT_MAX); }

	/** @brief returns a random float in the range [0, 1) */
	float NextFloat() { return ((NextInt() * 1.0f) / RANDINT_MAX); }

	/** @brief returns a random vector with length [0, 1] */
	float3 NextVector2D() { return (NextVector(0.0f)); }
	float3 NextVector(float y = 1.0f) {
		float3 ret;

		do {
			ret.x =  NextFloat() * 2.0f - 1.0f;
			ret.y = (NextFloat() * 2.0f - 1.0f) * y;
			ret.z =  NextFloat() * 2.0f - 1.0f;
		} while (ret.SqLength() > 1.0f);

		return ret;
	}

private:
	unsigned int randSeed;
};

#endif

