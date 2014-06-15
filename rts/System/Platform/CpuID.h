/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef CPUID_H
#define CPUID_H

#if defined(__GNUC__)
	#define _noinline __attribute__((__noinline__))
#else
	#define _noinline
#endif

namespace springproc {
	_noinline void ExecCPUID(unsigned int* a, unsigned int* b, unsigned int* c, unsigned int* d);
}

#endif // CPUID_H
