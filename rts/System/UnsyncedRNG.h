#ifndef UNSYNCEDRNG_H_
#define UNSYNCEDRNG_H_


class UnsyncedRNG
{
public:
	UnsyncedRNG();
	void Seed(unsigned seed);
	unsigned operator()();
	
	/** @brief returns a random number in the range [0, n) */
	int operator()(int n);

private:
	unsigned randSeed;
};

#endif /*UNSYNCEDRNG_H_*/
