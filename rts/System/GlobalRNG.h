/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RNG_H
#define _GLOBAL_RNG_H

#include <climits> // INT_MAX

#include "Sim/Misc/GlobalConstants.h" // RANDINT_MAX
#include "System/float3.h"

// crappy but fast LCG
class CGlobalSyncedRNG {
public:
	CGlobalSyncedRNG() { SetSeed(0, true); }

	// needed for random_shuffle
	int operator()(unsigned int N) { return (NextInt() % N); }

	int NextInt() { return (((randSeed = (randSeed * 214013L + 2531011L)) >> 16) & RANDINT_MAX); }
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
	int randSeed;

	/**
	* @brief initial random seed
	*
	* Holds the synced initial random seed
	*/
	int initRandSeed;
};


class CGlobalUnsyncedRNG {
public:
	CGlobalUnsyncedRNG(): randSeed(0) {}

	void Seed(unsigned int seed) { randSeed = seed; }

	/** @brief returns a random unsigned integer in the range [0, (UINT_MAX & 0x7FFF)) */
	unsigned int operator()() { int r = NextInt(); return *reinterpret_cast<unsigned*>(&r); }

	/** @brief returns a random integer in the range [0, (INT_MAX & 0x7FFF)) */
	int NextInt() { return (((randSeed = (randSeed * 214013L + 2531011L)) >> 16) & RANDINT_MAX); }

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

	/** @brief returns a random number in the range [0, n) */
	// (the range of RandInt() is limited to (INT_MAX & 0x7FFF))
	int operator()(int n) { return (NextInt() * n / ((INT_MAX & RANDINT_MAX) + 1)); }
	void operator = (const CGlobalUnsyncedRNG& urng) { Seed(urng.randSeed); }

private:
	unsigned int randSeed;
};

#endif

