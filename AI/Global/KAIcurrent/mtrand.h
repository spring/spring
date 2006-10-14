#ifndef MTRAND_H
#define MTRAND_H

class MTRand_int32 { // Mersenne Twister random number generator
public:
// default constructor: uses default seed only if this is the first instance
  MTRand_int32() { if (!init) seed(5489UL); init = true; }
// constructor with 32 bit int as seed
  MTRand_int32(unsigned int s) { seed(s); init = true; }
  void seed(unsigned int); // seed with 32 bit integer
// overload operator() to make this a generator (functor)
  unsigned int operator()() { return rand_int32(); }
  ~MTRand_int32() {} // destructor
protected: // used by derived classes, otherwise not accessible; use the ()-operator
  unsigned int rand_int32(); // generate 32 bit random integer
private:
  static const int n = 624, m = 397; // compile time constants
// the variables below are static (no duplicates can exist)
  static unsigned int state[n]; // state vector array
  static int p; // position in state array
  static bool init; // true if init function is called
// private functions used to generate the pseudo random numbers
  unsigned int twiddle(unsigned int, unsigned int); // used by gen_state()
  void gen_state(); // generate new state
// make copy constructor and assignment operator unavailable, they don't make sense
  MTRand_int32(const MTRand_int32&); // copy constructor not defined
  void operator=(const MTRand_int32&); // assignment operator not defined
};

// inline for speed, must therefore reside in header file
inline unsigned int MTRand_int32::twiddle(unsigned int u, unsigned int v) {
  return (((u & 0x80000000UL) | (v & 0x7FFFFFFFUL)) >> 1)
    ^ ((v & 1UL) ? 0x9908B0DFUL : 0x0UL);
}

inline unsigned int MTRand_int32::rand_int32() { // generate 32 bit random int
  if (p == n) gen_state(); // new state vector needed
// gen_state() is split off to be non-inline, because it is only called once
// in every 624 calls and otherwise irand() would become too big to get inlined
  unsigned int x = state[p++];
  x ^= (x >> 11);
  x ^= (x << 7) & 0x9D2C5680UL;
  x ^= (x << 15) & 0xEFC60000UL;
  return x ^ (x >> 18);
}
// generates double floating point numbers in the half-open interval [0, 1)
class MTRand : public MTRand_int32 {
public:
  MTRand() : MTRand_int32() {}
  MTRand(unsigned int seed) : MTRand_int32(seed) {} // The day 64 bit evil computers take over change this back to long
  ~MTRand() {}
  double operator()() {
    return static_cast<float>(rand_int32()) * (1. / 4294967296.); } // divided by 2^32
private:
  MTRand(const MTRand&); // copy constructor not defined
  void operator=(const MTRand&); // assignment operator not defined
};
#endif // MTRAND_H
