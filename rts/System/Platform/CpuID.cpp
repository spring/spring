/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#include "CpuID.h"
//#include <cstddef>
#ifdef _MSC_VER
	#include <intrin.h>
#endif


namespace springproc {
#if defined(__GNUC__)
	// function inlining breaks this
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
	#ifndef __APPLE__
		__asm__ __volatile__(
			"cpuid"
			: "=a" (*a), "=b" (*b), "=c" (*c), "=d" (*d)
			: "0" (*a)
		);
	#else
		#ifdef __x86_64__
			__asm__ __volatile__(
				"pushq %%rbx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popq %%rbx"
				: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#else
			__asm__ __volatile__(
				"pushl %%ebx\n\t"
				"cpuid\n\t"
				"movl %%ebx, %1\n\t"
				"popl %%ebx"
				: "=a" (*a), "=r" (*b), "=c" (*c), "=d" (*d)
				: "0" (*a)
			);
		#endif
	#endif
	}
#elif defined(_MSC_VER) && (_MSC_VER >= 1310)
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
		int features[4];
		__cpuid(features, *a);
		*a=features[0];
		*b=features[1];
		*c=features[2];
		*d=features[3];
	}
#else
	// no-op on other compilers
	void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d)
	{
	}
#endif
}
