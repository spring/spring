#include "UnsyncedRNG.h"

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
	return randSeed & 0x7FFF;
}

int UnsyncedRNG::operator()(int n)
{
	int randint = (*this)();
	// a simple gu->usRandInt() % n isn't random enough
	return randint * n / (INT_MAX + 1);
}
