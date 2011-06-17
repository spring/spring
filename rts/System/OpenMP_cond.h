/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef _OPEN_MP_COND_H
#define _OPEN_MP_COND_H

/*
 * Using this header allows to enable or disable the use of OpenMP
 * through the build-system, without getting warnings in eihter case.
 */

#ifdef _OPENMP
	#include <omp.h>
#else
	// For GCC, unknown pragma warnings are disabled globally
	// with "-Wno-unknown-pragmas".
	// On MSVC, we can disable unknown pragma warnings locally:
	#pragma warning (disable : 4068 )
#endif

#endif // _OPEN_MP_COND_H
