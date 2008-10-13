#ifndef MTRAND_H
#define MTRAND_H


// Mersenne Twister random number generator
class MTRand_int32 {
	public:
		// default constructor: uses default seed only if this is the first instance
		MTRand_int32() {
			if (!init) {
				seed(5489UL);
				init = true;
			}
		}
		// constructor with 32 bit int as seed
		MTRand_int32(unsigned int s) {
			seed(s); init = true;
		}
		virtual ~MTRand_int32() {
		}

		// seed with 32 bit integer
		void seed(unsigned int);

		// overload operator() to make this a generator (functor)
		unsigned int operator () () {
			return rand_int32();
		}

	// used by derived classes, otherwise not accessible; use the ()-operator
	protected:
		// generate 32 bit random integer
		unsigned int rand_int32();

	// private functions used to generate the pseudo random numbers
	private:
		// compile time constants
		static const int n = 624, m = 397;

		// state vector array
		static unsigned int state[n];
		// position in state array
		static int p;
		// true if init function is called
		static bool init;

		// used by gen_state()
		unsigned int twiddle(unsigned int, unsigned int);
		// generate new state
		void gen_state();

		// make copy constructor and assignment operator unavailable, they don't make sense

		// copy constructor not defined
		MTRand_int32(const MTRand_int32&);
		// assignment operator not defined
		void operator = (const MTRand_int32&);
};


// inline for speed, must therefore reside in header file
inline unsigned int MTRand_int32::twiddle(unsigned int u, unsigned int v) {
	return (((u & 0x80000000UL) | (v & 0x7FFFFFFFUL)) >> 1) ^ ((v & 1UL)? 0x9908B0DFUL: 0x0UL);
}

// generate 32 bit random int
inline unsigned int MTRand_int32::rand_int32() {
	if (p == n) {
		// new state vector needed
		gen_state();
	}

	// gen_state() is split off to be non-inline, because it is only called once
	// in every 624 calls and otherwise irand() would become too big to get inlined
  	unsigned int x = state[p++];
	x ^= (x >> 11);
	x ^= (x << 7) & 0x9D2C5680UL;
	x ^= (x << 15) & 0xEFC60000UL;

	return (x ^ (x >> 18));
}

// generates double floating point numbers in the half-open interval [0, 1)
class MTRand: public MTRand_int32 {
	public:
		MTRand(): MTRand_int32() {
		}
		// The day 64 bit evil computers take over change this back to long
		MTRand(unsigned int seed): MTRand_int32(seed) {
		}
		~MTRand() {
		}

		double operator () () {
			// divided by 2^32
			return static_cast<float>(rand_int32()) * (1. / 4294967296.);
		}

	private:
		// copy constructor not defined
		MTRand(const MTRand&);
		// assignment operator not defined
		void operator = (const MTRand&);
};


#endif
