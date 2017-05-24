/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RNG_H
#define _GLOBAL_RNG_H

#include <limits>

#include "Sim/Misc/GlobalConstants.h" // RANDINT_MAX
#include "System/float3.h"



struct PCG32Random {
public:
	typedef uint32_t res_type;
	typedef uint64_t val_type;

	PCG32Random(const val_type _val = def_val, const val_type _seq = def_seq) { seed(_val, _seq); }
	PCG32Random(const PCG32Random& rng) { *this = rng; }

	void seed(const val_type initval, const val_type initseq) {
		val = 0u;
		inc = (initseq << 1u) | 1u;

		next();
		val += initval;
		next();
	}

	res_type next() {
		const val_type oldval = val;

		// advance internal state
		val = oldval * 6364136223846793005ull + inc;

		// calculate output function (XSH RR), uses old state for max ILP
		const res_type xsh = ((oldval >> 18u) ^ oldval) >> 27u;
		const res_type rot = oldval >> 59u;
		return ((xsh >> rot) | (xsh << ((-rot) & 31)));
	}

	res_type bnext(const res_type bound) {
		const res_type threshold = -bound % bound;
		res_type r = 0;

		// generate a uniformly distributed number, r, where 0 <= r < bound
		for (r = next(); r < threshold; r = next());

		return (r % bound);
	}

public:
	static constexpr val_type def_val = 0x853c49e6748fea9bULL;
	static constexpr val_type def_seq = 0xda3e39cb94b95bdbULL;

private:
	val_type val;
	val_type inc;
};


template<bool synced> class CGlobalRNG {
public:
	void Seed(PCG32Random::val_type seed) { SetSeed(seed); }
	void SetSeed(PCG32Random::val_type seed, bool init = false) {
		assert(min() ==           0);
		assert(max() >= RANDINT_MAX);

		// use address of this object as sequence-id for unsynced RNG, modern systems have ASLR
		if (init) {
			pcg.seed(initSeed = seed, PCG32Random::val_type(this) * (1 - synced) + PCG32Random::def_seq * synced);
		} else {
			pcg.seed(lastSeed = seed, PCG32Random::val_type(this) * (1 - synced) + PCG32Random::def_seq * synced);
		}
	}

	PCG32Random::val_type GetInitSeed() const { return initSeed; }
	PCG32Random::val_type GetLastSeed() const { return lastSeed; }

	// needed for std::{random_}shuffle
	PCG32Random::res_type operator()(                       ) { return (pcg. next( )); }
	PCG32Random::res_type operator()(PCG32Random::res_type N) { return (pcg.bnext(N)); }

	static constexpr PCG32Random::res_type min() { return (std::numeric_limits<PCG32Random::res_type>::min()); }
	static constexpr PCG32Random::res_type max() { return (std::numeric_limits<PCG32Random::res_type>::max()); }

	PCG32Random::res_type NextInt(PCG32Random::res_type N = RANDINT_MAX) { return ((*this)(N)); }
	float NextFloat(PCG32Random::res_type N = RANDINT_MAX) { return ((NextInt(N) * 1.0f) / N); }

	float3 NextVector2D() { return (NextVector(0.0f)); }
	float3 NextVector(float y = 1.0f) {
		float3 ret;

		do {
			ret.x = (NextFloat() * 2.0f - 1.0f);
			ret.y = (NextFloat() * 2.0f - 1.0f) * y;
			ret.z = (NextFloat() * 2.0f - 1.0f);
		} while (ret.SqLength() > 1.0f);

		return ret;
	}

private:
	PCG32Random pcg;

	// initial and last-set seed
	PCG32Random::val_type initSeed = 0;
	PCG32Random::val_type lastSeed = 0;
};


// synced and unsynced RNG's no longer need to be different types
typedef CGlobalRNG<true> CGlobalSyncedRNG;
typedef CGlobalRNG<false> CGlobalUnsyncedRNG;

#endif

