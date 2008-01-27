#ifndef UNSYNCEDRNG_H_
#define UNSYNCEDRNG_H_


class UnsyncedRNG
{
public:
	UnsyncedRNG();
	void Seed(unsigned seed);
	unsigned operator()();

private:
	unsigned randSeed;
};

#endif /*UNSYNCEDRNG_H_*/
