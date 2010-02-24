/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef UNSYNCEDRNG_H_
#define UNSYNCEDRNG_H_

class UnsyncedRNG
{
public:
	UnsyncedRNG();
	void Seed(unsigned seed);
	
	/** @brief returns a random unsigned integer in the range [0, (UINT_MAX & 0x7FFF)) */
	unsigned operator()();
	
	/** @brief returns a random integer in the range [0, (INT_MAX & 0x7FFF)) */
	int RandInt();
	
	float RandFloat();
	
	/** @brief returns a random number in the range [0, n) */
	int operator()(int n);

private:
	unsigned randSeed;
};

#endif // UNSYNCEDRNG_H_

