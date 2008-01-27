#include "UnsyncedRNG.h"

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