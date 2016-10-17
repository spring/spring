/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCEDRNG_H_
#define UNSYNCEDRNG_H_

#include "System/float3.h"


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

};

#endif // UNSYNCEDRNG_H_

