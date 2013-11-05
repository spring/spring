/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

/**
 * @brief good_fpu_control_registers function
 *
 * Assertions on floating point unit control registers.
 * For now it only defines the good_fpu_control_registers() function.
 */

#ifndef _FPU_CHECK_H
#define _FPU_CHECK_H


extern void good_fpu_control_registers(const char* text);
extern void good_fpu_init();
extern void streflop_init_omp();

#if defined(__GNUC__)
	#define _noinline __attribute__((__noinline__))
#else
	#define _noinline
#endif

namespace springproc {
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);

	unsigned int GetProcMaxStandardLevel();
	unsigned int GetProcMaxExtendedLevel();
	unsigned int GetProcSSEBits();
}

#endif // !_FPU_CHECK_H
