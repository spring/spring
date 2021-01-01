/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _GLOBAL_RNG_H
#define _GLOBAL_RNG_H

#include <limits>

#include "lib/streflop/streflop_cond.h"
#include "System/float3.h"



#if 0
struct LCG16 {
public:
	typedef uint16_t res_type;
	typedef uint32_t val_type;

	LCG16(const val_type _val = def_val, const val_type _seq = def_seq) { seed(_val, _seq); }
	LCG16(const LCG16& rng) { *this = rng; }

	void seed(const val_type initval, const val_type initseq) {
		val = initval;
		seq = initseq;
	}

	res_type next() { return (((val = (val * 214013L + seq)) >> 16) & max_res); }
	res_type bnext(const res_type bound) { return (next() % bound); }

	val_type state() const { return val; }

public:
	static constexpr res_type min_res = 0;
	static constexpr res_type max_res = 0x7fff;
	static constexpr val_type def_val = 0;
	static constexpr val_type def_seq = 2531011L;

private:
	val_type val;
	val_type seq;
};
#endif

struct PCG32 {
public:
	typedef uint32_t res_type;
	typedef uint64_t val_type;

	PCG32(const val_type _val = def_val, const val_type _seq = def_seq) { seed(_val, _seq); }
	PCG32(const PCG32& rng) { *this = rng; }

	void seed(const val_type initval, const val_type initseq) {
		val = 0u;
		seq = (initseq << 1u) | 1u;

		next();
		val += initval;
		next();
	}

	res_type next() {
		const val_type oldval = val;

		// advance internal state
		val = oldval * 6364136223846793005ull + seq;

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

	val_type state() const { return val; }

public:
	static constexpr res_type min_res = std::numeric_limits<res_type>::min();
	static constexpr res_type max_res = std::numeric_limits<res_type>::max();
	static constexpr val_type def_val = 0x853c49e6748fea9bULL;
	static constexpr val_type def_seq = 0xda3e39cb94b95bdbULL;

private:
	val_type val;
	val_type seq;
};



template<typename RNG, bool synced> class CGlobalRNG {
public:
	typedef typename RNG::val_type rng_val_type;
	typedef typename RNG::res_type rng_res_type;

	static_assert(std::numeric_limits<float>::digits == 24, "sign plus mantissa bits should be 24");

	void Seed(rng_val_type seed) { SetSeed(seed); }
	void SetSeed(rng_val_type seed, bool init = false) {
		// use address of this object as sequence-id for unsynced RNG, modern systems have ASLR
		if (init) {
			gen.seed(initSeed = seed, static_cast<rng_val_type>(size_t(this)) * (1 - synced) + RNG::def_seq * synced);
		} else {
			gen.seed(lastSeed = seed, static_cast<rng_val_type>(size_t(this)) * (1 - synced) + RNG::def_seq * synced);
		}
	}

	rng_val_type GetInitSeed() const { return initSeed; }
	rng_val_type GetLastSeed() const { return lastSeed; }
	rng_val_type GetGenState() const { return (gen.state()); }

	// needed for std::{random_}shuffle
	rng_res_type operator()(              ) { return (gen. next( )); }
	rng_res_type operator()(rng_res_type N) { return (gen.bnext(N)); }

	static constexpr rng_res_type  min() { return RNG::min_res; }
	static constexpr rng_res_type  max() { return RNG::max_res; }
	static constexpr rng_res_type ndig() { return std::numeric_limits<float>::digits; }

	// [0, N)
	rng_res_type NextInt(rng_res_type N = max()) { return ((*this)(N)); }

	float NextFloat() { return (NextFloat01(1 << ndig())); }
	float NextFloat01(rng_res_type N) { return ((NextInt(N) * 1.0f) / N); } // [0,1) rounded to multiple of 1/N
	float NextFloat24() { return (math::ldexp(NextInt(1 << ndig()), -ndig())); } // [0,1) rounded to multiple of 1/(2^#digits)

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
	RNG gen;

	// initial and last-set seed
	rng_val_type initSeed = 0;
	rng_val_type lastSeed = 0;
};


// synced and unsynced RNG's no longer need to be different types
typedef CGlobalRNG<PCG32, true> CGlobalSyncedRNG;
typedef CGlobalRNG<PCG32, false> CGlobalUnsyncedRNG;

#endif

