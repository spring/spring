#include "MTRand.h"

unsigned int MTRand_int32::state[n] = {0x0UL};
int MTRand_int32::p = 0;
bool MTRand_int32::init = false;

void MTRand_int32::gen_state() { 
	for (int i = 0; i < (n - m); ++i)
		state[i] = state[i + m] ^ twiddle(state[i], state[i + 1]);
	for (int i = n - m; i < (n - 1); ++i)
		state[i] = state[i + m - n] ^ twiddle(state[i], state[i + 1]);

	state[n - 1] = state[m - 1] ^ twiddle(state[n - 1], state[0]);
	p = 0;
}

void MTRand_int32::seed(unsigned int s) {  // init by 32 bit seed
	state[0] = s & 0xFFFFFFFFUL; // for > 32 bit machines

	for (int i = 1; i < n; ++i) {
		state[i] = 1812433253UL * (state[i - 1] ^ (state[i - 1] >> 30)) + i;
		state[i] &= 0xFFFFFFFFUL; // for > 32 bit machines
	}

	p = n;
}
