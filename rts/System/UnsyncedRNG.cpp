/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "StdAfx.h"
#include "UnsyncedRNG.h"
#include "Sim/Misc/GlobalConstants.h"

#include <limits.h>

UnsyncedRNG::UnsyncedRNG() : randSeed(0)
{
}

void UnsyncedRNG::Seed(unsigned seed)
{
	randSeed ^= seed;
}

unsigned UnsyncedRNG::operator()()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return (randSeed >> 16) & RANDINT_MAX;
}

int UnsyncedRNG::RandInt()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return (randSeed >> 16) & RANDINT_MAX;
}

float UnsyncedRNG::RandFloat()
{
	randSeed = (randSeed * 214013L + 2531011L);
	return float((randSeed >> 16) & RANDINT_MAX) / RANDINT_MAX;
}

int UnsyncedRNG::operator()(int n)
{
	return RandInt() * n / ((INT_MAX & RANDINT_MAX) + 1); // the range of RandInt() is limited to (INT_MAX & 0x7FFF)
}
